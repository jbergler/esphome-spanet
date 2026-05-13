# SpaNet Component Architecture

## Overview

The SpaNet component bridges SpaNet spa controllers into the ESPHome/Home Assistant ecosystem. A SpaNet controller communicates via a proprietary serial protocol (38400 baud) that sends telemetry (temperature, power, pump status) and receives commands to change setpoints, pump modes, and heating state. This protocol combines byte framing, register parsing, state management, and hardware control in a tightly coordinated way, so the core design challenge is managing that complexity while keeping concerns clearly separated.

This component enforces **three explicit, testable layers**:
- A byte-level framing layer that knows nothing of protocol.
- A stateless parser that classifies messages and extracts fields.
- A stateful register cache that decodes raw fields into normalized types.

Above these layers sits the runtime orchestrator (`SpaNetComponent`) and entity implementations (climate, fan, sensors). This separation means you can test the framing layer without a full UART, the parser without ESPHome, and state decoders without runtime. It also means adding a new entity rarely touches the parser or framing logic, instead you subscribe to state and route user requests through the command queue.

---

## The Three Architectural Layers

### Layer 1: UartRxBuffer — Framing Only

**Responsibility**: Accumulate bytes from the UART hardware and emit newline-delimited strings.

The `UartRxBuffer` class in [uart_rx_buffer.h](components/spanet/uart_rx_buffer.h) is a simple byte accumulator. It has one public method: `feed(byte)`. Bytes are collected into an internal buffer; when a newline (`\n`) is encountered, the buffer is returned and cleared. Carriage returns (`\r`) are silently dropped.

**Architectural guarantee**: This layer contains zero protocol knowledge. It does not parse register labels, classify message types, or validate checksum formats. If the SpaNet protocol changes its framing (e.g., switches from newline-delimited to fixed-size frames), only this layer changes; parser and state logic remain untouched.

**What developers need to know**: If you're adding support for a different framing (e.g., fixed-size messages instead of newline-delimited), modify this layer only. If you're tempted to parse a field or classify a message here, move that logic to `SpaNetParser` instead.

---

### Layer 2: SpaNetParser — Stateless Classification and Parsing

**Responsibility**: Accept a single UART line and either (a) classify it as a state update, acknowledgment, or unknown, or (b) if it's a state line, parse the register fields into a typed struct.

The `SpaNetParser` class in [spanet_parser.h](components/spanet/spanet_parser.h) provides two static methods:

1. **`classify_message(line)`**: Routes a line as `kStateUpdate` (RF prefix or recognized register label like `R2`, `R3`), `kAck` (S or W prefix), or `kUnknown`.
2. **`parse_register_line(line)`**: Parses comma-delimited fields into one of several typed register structs (`RegisterR2`, `RegisterR3`, ..., `RegisterRG`). Each struct has field indices as compile-time constants (e.g., `RegisterR3::kSoftwareVersionIndex = 5`) and a static `from_fields()` method that validates field count before reading.

The parser **must be stateless**. It never remembers which registers it has seen before, never accumulates partial state, and never decides what to do with parsed data. It only answers: "What type is this message?" and "If it's a register line, what are the fields?"

**Architectural guarantee**: The parser is a pure function. Feed it the same line 1000 times, and it returns the same result every time. This makes it trivial to test (no mocks, no setup/teardown, no hidden state) and easy to reason about.

**What developers need to know**: If you're adding a new register type to the protocol (e.g., a new RH register with different fields), add a new struct to `SpaNetParser` with its field indices and a `from_fields()` method. If you need to track state (e.g., "I've seen this register once already, so now I should update the cache"), that logic lives in `RegisterStore`, not here.

---

### Layer 3: RegisterStore — Stateful Cache with Typed Accessors

**Responsibility**: Maintain the latest value of each register by label. When a parsed register arrives, overwrite the stored value. Provide typed decoders to convert raw field strings into normalized types (e.g., "385" → 38.5°C, "1-2-0324" → pump install state with capability flags).

The `RegisterStore` class in [spanet_state.h](components/spanet/spanet_state.h) and [spanet_state.cpp](components/spanet/spanet_state.cpp) owns all register types as member variables. Its main method is `update(line)`: parse the register line and overwrite only the register it references. Decoding happens in a separate `State` struct that is built on-demand by `RegisterStore::get_state()`. This struct contains normalized fields like `temperatures.water_c`, `climate.heating_active`, `pumps[i].current_raw_mode`, etc.

Examples of decoding: `parse_tenths_celsius("385")` → 38.5°C; `parse_pump_install_state("1-2-0324")` → struct with installed flag, supports_speed, supports_auto, and raw mode capabilities. Decoding happens only when `get_state()` is called, so the rest of the system works with clean, typed data.

**Architectural guarantee**: All "glue logic" that converts raw register fields into normalized types lives here. Entity implementations (climate, fan) never parse strings or validate field counts. They read from `State` and know types are correct.

**What developers need to know**: If adding a new sensor (e.g., water pressure from a new register field), add the raw field to the appropriate `Register*` struct, then add a decoder function in this file (following the pattern of `parse_tenths_celsius`). The `State` struct gains a new field, and entity callbacks automatically see it. No parser changes needed.

---

## Key Components

### SpaNetComponent — The Hub and Orchestrator

`SpaNetComponent` in [spanet.h](components/spanet/spanet.h) and [spanet.cpp](components/spanet/spanet.cpp) is the runtime heart. It owns all three layers (`UartRxBuffer`, `SpaNetParser`, `RegisterStore`), the command queue, and a debounce gate. It also manages all entity subscriptions.

**Responsibilities**:
- Initialize the UART hardware and layers in `setup()`.
- In `loop()` (called ~50 Hz), drain UART bytes and process command timeouts.
- Route incoming messages: first try to acknowledge them (commands in flight wait for acks), then classify them (state updates trigger debounced callbacks; unknown lines are logged and dropped).
- Maintain the invariant: **only one in-flight acked command at a time**. The command queue enforces this; the component respects it by not enqueuing write commands if one is pending.
- Provide request methods (`request_setpoint_temperature()`, `request_pump_mode()`) that entities call to enqueue write commands.

**Architectural guarantee**: Message routing is predictable and order-preserving. Acks are matched first (so the queue unblocks); state lines always trigger the same callback chain. State publication is debounced at 250 ms, so a burst of register lines (common when the spa wakes up) becomes one `State` notification.

**What developers need to know**: When adding a new entity, you subscribe to state callbacks via `add_on_state_callback()`. The component calls your callback with the latest `State` whenever the debounce gate fires. You publish entity state only if a field changed (ESPHome handles that; just call `this->publish_state()`). To enqueue a write command, call one of the request methods; the queue handles ack matching and timeouts.

---

### CommandQueue — Asynchronous Command Dispatch with SpaNet Semantics

The `CommandQueue` in [command_queue.h](components/spanet/command_queue.h) is a bounded FIFO queue (default 4 slots) that enforces one in-flight acked command at a time.

**Core methods**:
- **`enqueue(command)`**: Add a command to the queue. If the queue is empty, send immediately. If the queue is full or a duplicate RF poll is already queued, drop with a warning. Otherwise, queue and wait for in-flight to complete.
- **`acknowledge(ack_payload, out_matched)`**: Try to match an incoming ack against the in-flight command's expected ack. If matched, clear in-flight, send the next queued command, and return `kMatched`. If not matched or no in-flight, pass the message to the parser.
- **`expire_timed_out(now_ms)`**: Check if the in-flight command has aged past its timeout (typically 1000 ms). If so, clear it and unblock the next queued command.

**Architectural guarantee**: Write commands and RF polls are dispatched one-at-a-time. Duplicate RF polls are suppressed (common pattern: user triggers a refresh while one is pending). Commands are guaranteed to reach the device in the order they were enqueued. Timeouts prevent deadlocks if the device never acknowledges a command.

**What developers need to know**: Do not manipulate the queue directly. Use the request methods in `SpaNetComponent`; they validate inputs before enqueueing. The queue is bounded to prevent unbounded memory growth; if a command is dropped due to queue full, it's logged as a warning. Test feature additions involving writes by checking that acks are matched, next-in-queue commands are sent, and timeout unblocks properly (examples in [test_spanet_component/test_main.cpp](tests/test_spanet_component/test_main.cpp)).

---

### Entity Implementations — State Subscribers and Control Dispatchers

Entities (climate, fan, sensors) in [spanet_climate.cpp](components/spanet/spanet_climate.cpp), [spanet_fan.cpp](components/spanet/spanet_fan.cpp), and [sensor.py](components/spanet/sensor.py) follow a consistent pattern: subscribe to parent state, publish when fields change, and dispatch user requests back to the parent's request methods.

**Climate entity** ([spanet_climate.h](components/spanet/spanet_climate.h), [spanet_climate.cpp](components/spanet/spanet_climate.cpp)):
- Reads water temperature, setpoint, and heating active status from the normalized `State`.
- Publishes to ESPHome as `current_temperature`, `target_temperature`, and `action`.
- On user input (e.g., setpoint change), calls `parent_->request_setpoint_temperature()` to enqueue a write.

**Fan entity** ([spanet_fan.h](components/spanet/spanet_fan.h), [spanet_fan.cpp](components/spanet/spanet_fan.cpp)):
- Maps pump capabilities (install state, supported speeds, auto mode) declared in the RG register to ESPHome fan speed and preset traits.
- Reads current pump mode and syncs it back to user via speed or preset.
- On user input (speed or mode change), calls `parent_->request_pump_mode()` to enqueue a write.

**Sensor entities** ([sensor.py](components/spanet/sensor.py), [text_sensor.py](components/spanet/text_sensor.py)):
- Read-only: water temperature, setpoint, voltage, current, power, controller model/serial/firmware.
- No request path; just subscribe and publish.

**Architectural guarantee**: Entities are decoupled from parser and UART logic. They only care about the normalized `State` type. Adding a new read-only sensor is as simple as adding a field to `State` (in `RegisterStore`) and creating a sensor entity that reads it.

---

## Data Flow

### Inbound: Device → State → Entities

When the spa controller emits data:

```
Controller:
  RF:
  ,R2,0,239,40,...
  ,R3,10,1,4,...
  ,R5,0,1,...
  
  ↓ UART hardware → UartRxBuffer.feed(byte)
  ↓ Emits "RF:" as one message, then ",R2,0,239,40,..." as another
  
  ↓ SpaNetComponent.loop() calls on_uart_message_() for each line
  ↓ First line "RF:": classify → kStateUpdate
  ↓ on_state_update_message_() parses "RF:" as start of a state burst
  
  ↓ Second line ",R2,...": classify → kStateUpdate
  ↓ RegisterStore.update() parses R2 fields into RegisterR2 struct
  ↓ state_update_debounce_.try_arm() (250 ms gate)
  
  ↓ [Lines for R3, R5, RG arrive in rapid succession]
  ↓ Each calls RegisterStore.update() to overwrite that register
  
  ↓ [Debounce timer fires after 250 ms of no new lines]
  ↓ notify_state_update_() called
  ↓ SpaNetComponent calls all registered state callbacks with new State
  
  ↓ Climate entity callback: publishes water_c, setpoint_c, heating_active
  ↓ Fan entity callback: publishes pump speed, auto mode status
  ↓ Sensor entity callbacks: publish voltage, current, temperature, etc.
  
Result: Home Assistant sees updated entity stateso
```

---

### Outbound: User Request → Command Queue → Device

When a user changes entity state (e.g., sets climate target to 38°C):

```
Home Assistant user sets climate.target_temperature = 38°C

↓ Climate entity control() method validates and calls parent_->request_setpoint_temperature(38.0)
  ↓ Quantize 38.0 to nearest 0.2°C (spa controller's grid): 38.0 → 380 tenths
  ↓ Validate range [50, 410] tenths (5–41°C)
  ↓ enqueue_command_(QueuedCommand{
      kind: kSetpointWrite,
      payload: "W40:380",
      expected_ack: "380",
      timeout_ms: 1000
    })

↓ CommandQueue.enqueue(): queue is empty, so send immediately
↓ UART writes "W40:380\n"

Controller receives write command, updates setpoint, and replies: "380"

↓ SpaNetComponent.loop() calls on_uart_message_("380")
  ↓ classify_message("380") → kAck (numeric response to W40:)
  ↓ command_queue_->acknowledge("380") → kMatched
  ↓ In-flight command cleared
  ↓ enqueue_command_(QueuedCommand{kind: kRfPoll, ...})  ← automatic refresh

↓ CommandQueue.enqueue(): send "RF\n" immediately (queue empty)

Controller receives RF poll and responds with new state (R2, R3, R5, RG, ...)

↓ [Inbound flow repeats; state updates with new setpoint]
↓ Climate entity publishes updated target_temperature to Home Assistant
```

**Key insight**: Write commands automatically trigger an RF refresh poll. This ensures the user sees the new setpoint reflected in Home Assistant within ~500 ms (RF timeout) or ~250 ms (debounce), even if the controller doesn't spontaneously emit state.

---

## Adding a New Feature: Control Lights

This section walks through how you would add a new entity type—say, control lights on/off—to understand how the architecture makes it easy.

### Step 1: Define the Python Schema

Create [lights.py](components/spanet/lights.py) following the pattern in [climate.py](components/spanet/climate.py) and [fan.py](components/spanet/fan.py):

```python
import esphome.codegen as cg
from esphome.components import light
import esphome.config_validation as cv
from esphome.const import CONF_ID

CONF_SPANET_ID = "spanet_id"
lights_ns = cg.esphome_ns.namespace("lights")
SpaNetLight = lights_ns.class_("SpaNetLight", light.LightOutput, cg.Component)

CONFIG_SCHEMA = light.light_schema(SpaNetLight).extend({
    cv.GenerateID(CONF_SPANET_ID): cv.use_id(spanet_ns.class_("SpaNetComponent")),
})

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await light.register_light(var, config)
    await cg.register_component(var, config)
    parent = await cg.get_variable(config[CONF_SPANET_ID])
    cg.add(parent.add_light_entity(var))
```

Register this schema in [__init__.py](components/spanet/__init__.py).

### Step 2: Create the C++ Entity Class

Create [spanet_light.h](components/spanet/spanet_light.h) and [spanet_light.cpp](components/spanet/spanet_light.cpp) following [spanet_climate.h](components/spanet/spanet_climate.h):

- Inherit from `light::LightOutput` and `Component`
- Constructor takes parent `SpaNetComponent*`
- `setup()`: Subscribe to parent state via `parent_->add_on_state_callback()`
- `write_state(call)`: Handle user on/off requests; enqueue via parent

```cpp
class SpaNetLight : public light::LightOutput, public Component {
 public:
  SpaNetLight(SpaNetComponent *parent) : parent_(parent) {}
  
  void setup() override {
    parent_->add_on_state_callback([this](const State &state) {
      handle_state_update_(state);
    });
  }
  
  light::LightTraits get_traits() const override {
    auto traits = light::LightTraits();
    traits.set_supports_brightness(false);
    traits.set_supports_color_mode(light::ColorMode::ON_OFF);
    return traits;
  }
  
  void write_state(light::LightCall const &call) override {
    if (call.get_state().has_value()) {
      bool on = call.get_state().value();
      parent_->request_light_control(on);  // New request method
    }
  }

 private:
  void handle_state_update_(const State &state) {
    if (state.lights.light_on.has_value()) {
      this->state = state.lights.light_on.value();
      this->publish_state();
    }
  }
  
  SpaNetComponent *parent_;
};
```

### Step 3: Add Register Decoding

If lights have state in a register (e.g., a new RH register), add the register struct to [spanet_parser.h](components/spanet/spanet_parser.h):

```cpp
struct RegisterRH {
  static constexpr size_t kLightsOnIndex = 3;
  std::string lights_on;
  
  static std::optional<RegisterRH> from_fields(const std::vector<std::string> &fields) {
    if (fields.size() <= kLightsOnIndex) return std::nullopt;
    RegisterRH out;
    out.lights_on = fields[kLightsOnIndex];
    return out;
  }
};
```

Then in [spanet_state.h](components/spanet/spanet_state.h) and [spanet_state.cpp](components/spanet/spanet_state.cpp), add fields to decode:

```cpp
struct State {
  struct {
    std::optional<bool> light_on;
  } lights;
};
```

And in `RegisterStore::get_state()`, decode the register:

```cpp
if (registers_.rh) {
  auto lights_on = registers_.rh->lights_on == "1";
  state.lights.light_on = lights_on;
}
```

### Step 4: Add the Request Method to SpaNetComponent

In [spanet.h](components/spanet/spanet.h), add:

```cpp
void request_light_control(bool on) {
  std::string payload = on ? "WL1:1" : "WL1:0";
  enqueue_command_(QueuedCommand{
    .kind = CommandKind::kLightWrite,
    .payload = payload,
    .expected_ack = on ? "1" : "0",
    .timeout_ms = 1000,
  });
}
```

### Step 5: Test the Command Queue Behavior

Add a test in [test_spanet_component/test_main.cpp](tests/test_spanet_component/test_main.cpp) to verify:
- Enqueuing a light command succeeds
- Ack "1" matches and unblocks the next queued command
- Timeout advances if no ack arrives

Example pattern:

```cpp
TEST(SpaNetComponent, LightOnWriteAndAck) {
  auto mock_write = [](...) { /* capture UART writes */ };
  SpaNetComponent component;
  
  component.request_light_control(true);
  // Verify "WL1:1\n" was written
  
  component.on_uart_message_("1");
  // Verify ack was matched, next queued command sent
}
```

### Step 6: Test Register Parsing

Add a test in [test_spanet_parser/test_main.cpp](tests/test_spanet_parser/test_main.cpp):

```cpp
TEST(SpaNetParser, ParseRegisterRH) {
  std::vector<std::string> fields = {"RH", "0", "0", "1"};
  auto rh = RegisterRH::from_fields(fields);
  ASSERT_TRUE(rh.has_value());
  EXPECT_EQ(rh->lights_on, "1");
}
```

And test state decoding in [test_spanet_state/test_main.cpp](tests/test_spanet_state/test_main.cpp):

```cpp
TEST(RegisterStore, DecodeLightsFromRH) {
  RegisterStore store;
  store.update(AnyRegisterLine::Rh(...));
  auto state = store.get_state();
  EXPECT_TRUE(state.lights.light_on.has_value());
}
```

### Step 7: Validate

Run the two required validation commands from the repository root:

```bash
pio test -e cpp-test
esphome compile tests/spanet/test.esp32-idf.yaml
```

The first runs all unit tests; the second ensures the component compiles against ESPHome. Both must pass before changes are merged.

---

## Testing Strategy

The component uses a layered testing approach: test each layer in isolation, then test integration.

- **UART framing tests** ([test_uart_rx/test_main.cpp](tests/test_uart_rx/test_main.cpp)): Verify byte accumulation, newline splitting, and carriage return handling. No protocol knowledge.
  
- **Parser tests** ([test_spanet_parser/test_main.cpp](tests/test_spanet_parser/test_main.cpp)): Verify message classification (state vs ack) and register field extraction. No ESPHome includes, no runtime.
  
- **State decoding tests** ([test_spanet_state/test_main.cpp](tests/test_spanet_state/test_main.cpp)): Verify register parsing, field bounds checking, and conversion to normalized types (e.g., "385" → 38.5°C). No ESPHome, no UART.
  
- **Component integration tests** ([test_spanet_component/test_main.cpp](tests/test_spanet_component/test_main.cpp)): Verify message routing, ack matching, command queue behavior, debounce timing, and entity callbacks. Mock UART writes and reads to avoid hardware.

**When to add tests**: Whenever you change a parser, register structure, or queue behavior, add a focused test at the appropriate layer. When adding a new entity, test its request method's ack matching and queue interaction in the component tests.

---

## Glossary

- **RF poll**: A refresh request sent to the controller, prompting it to emit current register state (R2, R3, etc.). Fire-and-forget; the controller decides when to respond.
  
- **Ack (acknowledgment)**: A numeric response from the controller confirming a write command was received. E.g., "380" confirms "W40:380" (setpoint write).
  
- **Register**: A labeled group of comma-delimited fields sent by the controller (R2, R3, R5, RG, etc.). Each register contains related telemetry (R2 = temperatures and relay states; R3 = model and software version; RG = pump capabilities).
  
- **Tenths**: Temperature values encoded as integers representing tenths of degrees. E.g., "385" = 38.5°C. Used for both setpoint commands and setpoint telemetry.
  
- **Debounce gate**: A 250 ms timer that batches rapid state updates. When a register update arrives, the gate is armed; if no further updates arrive within 250 ms, entity callbacks are invoked once with the latest state. Prevents callback chatter during multi-register bursts.

- **In-flight command**: A command sent to the device that is waiting for an acknowledgment. Only one is allowed at a time; the command queue enforces this.

---

## References

For implementation guidance, see [AGENTS.md](AGENTS.md). For details on command queue semantics, timeout behavior, and bounded queue constraints, see the "Command Queue Checklist" in that file. For protocol details, see the [docs/espyspa-register-index-map.md](docs/espyspa-register-index-map.md) and [docs/spanet-parity-todo.md](docs/spanet-parity-todo.md).
