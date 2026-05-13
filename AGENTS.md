# AGENTS

Agent guidance for this repository. Keep this as a concise subset of `DESIGN.md`.

## Project Scope

- Repo type: ESPHome external component.
- Main code: `components/spanet`.
- Runtime hub: `SpaNetComponent` in `components/spanet/spanet.h` and `components/spanet/spanet.cpp`.
- Tests: GoogleTest (`pio test`) and ESPHome compile validation (`tests/spanet/test.esp32-idf.yaml`).

## Core Architecture Contracts

Protocol handling has three strict layers:

1. `UartRxBuffer`
   - Byte framing only.
   - Emits one message per newline.
   - No protocol parsing.

2. `SpaNetParser`
   - Stateless line classification and single-register parsing.
   - `classify_message`: state update vs ack vs unknown.
   - `parse_register_line`: parses one register line into parsed register content.

3. `RegisterStore`
   - Stateful latest-value register cache.
   - `update(line)` overwrites only the referenced register.
   - Typed state decoding/accessors read from stored fields.

Never mix responsibilities between these layers.

## Runtime Behavior To Preserve

- In `SpaNetComponent::on_uart_message_`:
  - First pass: `CommandQueue::acknowledge`
  - Unmatched/non-ack lines: classify and route
  - State lines: `on_state_update_message_`
  - Unknown lines: warn and drop
- Keep state publication debounced through `state_update_debounce_`.

## Command Queue Invariants

When changing `command_queue.h` or enqueue behavior:

1. Preserve duplicate RF poll suppression.
2. Preserve one in-flight acked command.
3. Preserve timeout advancement (expired in-flight unblocks next).
4. Preserve bounded queue behavior and explicit queue-full drops.
5. Add/update tests in `tests/test_spanet_component/test_main.cpp`.

## Parser/Register Safety

When changing parser or register decoding:

1. Check field bounds before indexing (`fields.size()` guards).
2. Keep index constants for non-trivial layouts.
3. Add tests for valid parse, short payload, and overwrite behavior.
4. Keep unknown-label handling behavior intact where applicable.

## Python vs C++ Responsibilities

- Python (`components/spanet/*.py`): schema and `to_code` wiring.
- C++ (`components/spanet/*.h/.cpp`): runtime behavior, parsing, state mapping.
- New entity work usually needs updates in both Python and C++.

## Adding New Entities (Checklist)

1. Add Python schema and `to_code` wiring.
2. Add/extend C++ request path in `SpaNetComponent` for writable entities.
3. Map register fields into normalized component state.
4. Subscribe with `add_on_state_callback` and publish on meaningful change.
5. Add focused tests for queue/ack behavior and state mapping.
6. Use existing fan/climate implementations as reference patterns.

## Required Validation Before Finishing

Run from repo root:

1. `pio test -e cpp-test`
2. `esphome compile tests/spanet/test.esp32-idf.yaml`

If parser/register/UART behavior changed, update parser/UART tests as needed:
- `tests/test_spanet_parser/test_main.cpp`
- `tests/test_uart_rx/test_main.cpp`

## Working Style

1. Prefer narrow, behavior-preserving edits.
2. Follow existing naming/style in touched files.
3. Keep comments concise and only for non-obvious logic.
4. Do not reintroduce `std::deque` in runtime command paths.
5. Summarize behavior changes, tests run, and residual risks.


## Tasks to remember
- When adding new entities, update `espa-mini-v1.yaml` to ensure the default build includes them.
- When planning support for new functionality, research the implementation in https://github.com/wayne-love/espyspa to understand protocol, quirks, edge cases, etc.