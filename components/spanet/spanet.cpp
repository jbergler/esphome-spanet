#include "spanet.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#include <cmath>
#include <cstdlib>

namespace esphome::spanet {

static const char *const TAG = "spanet";
static constexpr const char *const STATE_UPDATE_DEBOUNCE_TIMEOUT = "state_update_debounce";
static constexpr uint32_t STATE_UPDATE_DEBOUNCE_MS = 250;
static constexpr uint32_t COMMAND_TIMEOUT_MS = 1000;
static constexpr float SETPOINT_MIN_C = 5.0f;
static constexpr float SETPOINT_MAX_C = 41.0f;
static constexpr size_t MAX_QUEUED_COMMANDS = 4;

// For speed_type=2 pumps, controller command values 2/3 are inverted compared
// to observed readback raw modes in R5. Keep readback semantics in state/fan
// code and normalize only the outbound S22..S26 payload here.
static int encode_pump_command_mode(const PumpStatus &pump, int desired_raw_mode) {
  if (pump.speed_type == 2 && (desired_raw_mode == 2 || desired_raw_mode == 3)) {
    return desired_raw_mode == 2 ? 3 : 2;
  }
  return desired_raw_mode;
}

void SpaNetComponent::setup() {
  ESP_LOGI(TAG, "Setting up dummy SpaNET component");
  this->check_uart_settings(38400);

  this->command_queue_ =
      std::make_unique<CommandQueue>([this](const std::string &command) { this->send_uart_command_(command); },
                                     []() { return millis(); }, MAX_QUEUED_COMMANDS);

  this->add_on_state_callback([this](const State &state) {
    const auto &controller = state.controller_status;
    const auto &temperatures = state.temperatures;
    const auto &power = state.power;

    if (this->sen_controller_model_ != nullptr && !state.controller_status.model.empty()) {
      if (this->sen_controller_model_->get_raw_state() != controller.model) {
        this->sen_controller_model_->publish_state(controller.model);
      }
    }

    if (this->sen_controller_fw_version_ != nullptr && !controller.software_version.empty()) {
      if (this->sen_controller_fw_version_->get_raw_state() != controller.software_version) {
        this->sen_controller_fw_version_->publish_state(controller.software_version);
      }
    }

    if (this->sen_controller_serial_ != nullptr && !controller.serial_number.empty()) {
      if (this->sen_controller_serial_->get_raw_state() != controller.serial_number) {
        this->sen_controller_serial_->publish_state(controller.serial_number);
      }
    }

    if (this->sen_water_temperature_ != nullptr && temperatures.water_c.has_value()) {
      const float next_value = temperatures.water_c.value();
      if (std::isnan(this->sen_water_temperature_->state) || this->sen_water_temperature_->state != next_value) {
        this->sen_water_temperature_->publish_state(next_value);
      }
    }

    if (this->sen_setpoint_temperature_ != nullptr && temperatures.setpoint_c.has_value()) {
      const float next_value = temperatures.setpoint_c.value();
      if (std::isnan(this->sen_setpoint_temperature_->state) || this->sen_setpoint_temperature_->state != next_value) {
        this->sen_setpoint_temperature_->publish_state(next_value);
      }
    }

    if (this->sen_heater_temperature_ != nullptr && temperatures.heater_c.has_value()) {
      const float next_value = temperatures.heater_c.value();
      if (std::isnan(this->sen_heater_temperature_->state) || this->sen_heater_temperature_->state != next_value) {
        this->sen_heater_temperature_->publish_state(next_value);
      }
    }

    if (this->sen_case_temperature_ != nullptr && temperatures.case_c.has_value()) {
      const float next_value = temperatures.case_c.value();
      if (std::isnan(this->sen_case_temperature_->state) || this->sen_case_temperature_->state != next_value) {
        this->sen_case_temperature_->publish_state(next_value);
      }
    }

    if (this->sen_mains_voltage_ != nullptr && power.mains_voltage_v.has_value()) {
      const float next_value = power.mains_voltage_v.value();
      if (std::isnan(this->sen_mains_voltage_->state) || this->sen_mains_voltage_->state != next_value) {
        this->sen_mains_voltage_->publish_state(next_value);
      }
    }

    if (this->sen_mains_current_ != nullptr && power.mains_current_a.has_value()) {
      const float next_value = power.mains_current_a.value();
      if (std::isnan(this->sen_mains_current_->state) || this->sen_mains_current_->state != next_value) {
        this->sen_mains_current_->publish_state(next_value);
      }
    }

    if (this->sen_instant_power_ != nullptr && power.instant_power_w.has_value()) {
      const float next_value = power.instant_power_w.value();
      if (std::isnan(this->sen_instant_power_->state) || this->sen_instant_power_->state != next_value) {
        this->sen_instant_power_->publish_state(next_value);
      }
    }

    if (this->sen_total_energy_ != nullptr && power.total_energy_kwh.has_value()) {
      const float next_value = power.total_energy_kwh.value();
      if (std::isnan(this->sen_total_energy_->state) || this->sen_total_energy_->state != next_value) {
        this->sen_total_energy_->publish_state(next_value);
      }
    }
  });

  // Trigger an initial poll immediately; PollingComponent handles recurring
  // polls.
  this->update();
}

void SpaNetComponent::loop() {
  while (this->available() > 0) {
    uint8_t byte;
    if (!this->read_byte(&byte)) {
      break;
    }

    auto maybe_message = this->rx_buffer_.feed(byte);
    if (maybe_message.has_value()) {
      this->on_uart_message_(maybe_message.value());
    }
  }

  this->process_command_timeouts_(millis());
}

void SpaNetComponent::on_uart_message_(const std::string &message) {
  ESP_LOGI(TAG, "UART RX: '%s'", message.c_str());

  if (this->command_queue_ == nullptr) {
    return;
  }

  InFlightCommand matched_command;
  switch (this->command_queue_->acknowledge(message, &matched_command)) {
    case AckResult::kNoInFlightCommand:
      break;
    case AckResult::kUnmatchedAck:
      ESP_LOGV(TAG, "Message '%s' did not match expected ack for in-flight command", message.c_str());
      break;
    case AckResult::kMatched:
      if (matched_command.kind == CommandKind::kSetpointWrite || matched_command.kind == CommandKind::kPumpWrite ||
          matched_command.kind == CommandKind::kLightToggle || matched_command.kind == CommandKind::kLightBrightness ||
          matched_command.kind == CommandKind::kLightColor || matched_command.kind == CommandKind::kLightEffectMode ||
          matched_command.kind == CommandKind::kLightEffectSpeed) {
        this->enqueue_command_(QueuedCommand{
            .kind = CommandKind::kRfPoll,
            .payload = "RF",
            .expected_ack = "RF:",
            .timeout_ms = 500,
        });
      }
      return;
  }

  switch (SpaNetParser::classify_message(message)) {
    case MessageType::kStateUpdate:
      this->on_state_update_message_(message);
      break;
    case MessageType::kUnknown:
      ESP_LOGW(TAG, "Ignoring unknown message '%s'", message.c_str());
      break;
  }
}

void SpaNetComponent::on_state_update_message_(const std::string &message) {
  if (!this->register_store_.update(message)) {
    ESP_LOGW(TAG, "Failed to update register store for SpaNET state payload");
    return;
  }

  if (!this->state_update_debounce_.try_arm()) {
    return;
  }

  this->set_timeout(STATE_UPDATE_DEBOUNCE_TIMEOUT, STATE_UPDATE_DEBOUNCE_MS, [this]() {
    this->state_update_debounce_.disarm();
    this->notify_state_update_(this->register_store_.get_state());
  });
}

void SpaNetComponent::update() {
  ESP_LOGV(TAG, "Polling for state");
  this->enqueue_command_(QueuedCommand{
      .kind = CommandKind::kRfPoll,
      .payload = "RF",
      .expected_ack = "RF:",
      .timeout_ms = 500,
  });
}

void SpaNetComponent::notify_state_update_(const State &state) {
  for (auto &callback : this->state_callbacks_) {
    callback(state);
  }
}

void SpaNetComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "SpaNET dummy component");
  LOG_UPDATE_INTERVAL(this);
  if (this->sen_controller_model_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Model: %s", this->sen_controller_model_->get_name().c_str());
  }
  if (this->sen_controller_serial_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Serial: %s", this->sen_controller_serial_->get_name().c_str());
  }
  if (this->sen_controller_fw_version_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Firmware Version: %s", this->sen_controller_fw_version_->get_name().c_str());
  }
  if (this->sen_water_temperature_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Water Temperature: %s", this->sen_water_temperature_->get_name().c_str());
  }
  if (this->sen_setpoint_temperature_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Setpoint Temperature: %s", this->sen_setpoint_temperature_->get_name().c_str());
  }
  if (this->sen_heater_temperature_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Heater Temperature: %s", this->sen_heater_temperature_->get_name().c_str());
  }
  if (this->sen_case_temperature_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Case Temperature: %s", this->sen_case_temperature_->get_name().c_str());
  }
  if (this->sen_mains_voltage_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Mains Voltage: %s", this->sen_mains_voltage_->get_name().c_str());
  }
  if (this->sen_mains_current_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Mains Current: %s", this->sen_mains_current_->get_name().c_str());
  }
  if (this->sen_instant_power_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Instant Power: %s", this->sen_instant_power_->get_name().c_str());
  }
  if (this->sen_total_energy_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Total Energy: %s", this->sen_total_energy_->get_name().c_str());
  }
}

void SpaNetComponent::enqueue_command_(QueuedCommand command) {
  if (this->command_queue_ == nullptr) {
    return;
  }

  const CommandKind kind = command.kind;
  const std::string payload = command.payload;
  switch (this->command_queue_->enqueue(std::move(command))) {
    case EnqueueResult::kEnqueued:
      return;
    case EnqueueResult::kDroppedDuplicateRfPoll:
      ESP_LOGV(TAG, "Dropping duplicate RF poll while one is queued or in-flight");
      return;
    case EnqueueResult::kDroppedQueueFull:
      ESP_LOGW(TAG, "Dropping command due to full queue (max=%u): cmd=%s", static_cast<unsigned>(MAX_QUEUED_COMMANDS),
               payload.c_str());
      return;
  }
}

void SpaNetComponent::process_command_timeouts_(uint32_t now_ms) {
  if (this->command_queue_ == nullptr) {
    return;
  }

  InFlightCommand timed_out_command;
  uint32_t age_ms = 0;
  if (!this->command_queue_->expire_timed_out(now_ms, &timed_out_command, &age_ms)) {
    return;
  }

  ESP_LOGW(TAG, "Command timed out after %u ms: payload=%s", age_ms, timed_out_command.payload.c_str());

  if (timed_out_command.kind == CommandKind::kSetpointWrite) {
    this->awaiting_setpoint_reconcile_tenths_.reset();
  } else if (timed_out_command.kind == CommandKind::kLightToggle) {
    this->pending_light_toggle_state_.reset();
  }
}

void SpaNetComponent::send_uart_command_(const std::string &command) {
  ESP_LOGI(TAG, "UART TX: %s", command.c_str());
  this->write_str(command.c_str());
}

// Temperature Setpoint

bool SpaNetComponent::request_setpoint_temperature(float target_c) {
  auto maybe_target_tenths = this->quantize_and_encode_setpoint_(target_c);
  if (!maybe_target_tenths.has_value()) {
    ESP_LOGW(TAG, "Rejected invalid/out-of-range setpoint %.2fC", target_c);
    return false;
  }

  this->enqueue_command_(QueuedCommand{
      .kind = CommandKind::kSetpointWrite,
      .payload = "W40:" + std::to_string(maybe_target_tenths.value()),
      .expected_ack = std::to_string(maybe_target_tenths.value()),
      .timeout_ms = COMMAND_TIMEOUT_MS,
  });
  return true;
}

std::optional<int> SpaNetComponent::quantize_and_encode_setpoint_(float target_c) {
  if (std::isnan(target_c)) {
    return std::nullopt;
  }

  const float quantized_c = std::round(target_c * 5.0f) / 5.0f;
  if (quantized_c < SETPOINT_MIN_C || quantized_c > SETPOINT_MAX_C) {
    return std::nullopt;
  }

  return static_cast<int>(std::lround(quantized_c * 10.0f));
}

// Pump controls
bool SpaNetComponent::request_pump_mode(uint8_t pump_index, int raw_mode) {
  if (pump_index < 1 || pump_index > 5) {
    ESP_LOGW(TAG, "Rejected invalid pump index %u", pump_index);
    return false;
  }

  if (raw_mode < 0 || raw_mode > 4) {
    ESP_LOGW(TAG, "Rejected invalid pump mode %d for pump %u", raw_mode, pump_index);
    return false;
  }

  const auto &pump = this->register_store_.get_state().pumps[pump_index - 1];
  if (!pump.installed || !pump.capabilities_valid) {
    ESP_LOGW(TAG, "Rejected pump%u command while pump is unavailable", pump_index);
    return false;
  }

  if (!pump.supports_raw_mode[static_cast<size_t>(raw_mode)]) {
    ESP_LOGW(TAG, "Rejected unsupported pump%u mode %d", pump_index, raw_mode);
    return false;
  }

  const int command_mode = encode_pump_command_mode(pump, raw_mode);
  if (command_mode != raw_mode) {
    ESP_LOGV(TAG, "Pump%u translating desired mode %d to wire mode %d (speed_type=%d)", pump_index, raw_mode,
             command_mode, pump.speed_type);
  }

  const int command_family = 21 + static_cast<int>(pump_index);
  const std::string command_prefix = "S" + std::to_string(command_family);
  this->enqueue_command_(QueuedCommand{
      .kind = CommandKind::kPumpWrite,
      .payload = command_prefix + ":" + std::to_string(command_mode),
      .expected_ack = command_prefix + "-OK",
      .timeout_ms = COMMAND_TIMEOUT_MS,
  });
  return true;
}

// Helper methods for light control
uint8_t SpaNetComponent::esphome_brightness_to_device_(uint8_t esphome_0_255) {
  // Convert ESPHome brightness (0-255) to device scale (1-5)
  // 0-50 → 1, 51-102 → 2, 103-154 → 3, 155-205 → 4, 206-255 → 5
  return std::clamp(static_cast<uint8_t>(1 + (esphome_0_255 / 51)), uint8_t(1), uint8_t(5));
}

uint8_t SpaNetComponent::device_brightness_to_esphome_(uint8_t device_1_5) {
  // Convert device brightness (1-5) to ESPHome brightness (0-255)
  // 1 → 0, 2 → 51, 3 → 102, 4 → 153, 5 → 204
  if (device_1_5 < 1 || device_1_5 > 5) {
    return 0;
  }
  return (device_1_5 - 1) * 51;
}

uint8_t SpaNetComponent::hue_to_color_index_(uint16_t hue_degrees) {
  // Convert ESPHome hue (0-360°) to device color index
  // Find the colorMap entry for the closest hue step
  // Each step is 15°, so hue_index = (hue_degrees / 15) % 25
  uint16_t hue_index = (hue_degrees / 15) % 25;
  return LIGHT_COLOR_MAP[hue_index];
}

uint16_t SpaNetComponent::color_index_to_hue_(uint8_t color_index) {
  // Reverse map: find hue for a given color index by searching colorMap
  for (size_t i = 0; i < LIGHT_COLOR_MAP.size(); ++i) {
    if (LIGHT_COLOR_MAP[i] == color_index) {
      return i * 15;  // Return hue in degrees
    }
  }
  // If not found, return 0 (red)
  return 0;
}

// Light controls
bool SpaNetComponent::request_light_toggle(bool desired_state) {
  this->pending_light_toggle_state_ = desired_state;
  this->enqueue_command_(QueuedCommand{
      .kind = CommandKind::kLightToggle,
      .payload = "W14",
      .expected_ack = "W14",
      .timeout_ms = COMMAND_TIMEOUT_MS,
  });
  return true;
}

bool SpaNetComponent::request_light_brightness(uint8_t esphome_brightness) {
  uint8_t device_brightness = this->esphome_brightness_to_device_(esphome_brightness);
  const std::string device_brightness_str = std::to_string(device_brightness);
  this->enqueue_command_(QueuedCommand{
      .kind = CommandKind::kLightBrightness,
      .payload = "S08:" + device_brightness_str,
      .expected_ack = device_brightness_str,
      .timeout_ms = COMMAND_TIMEOUT_MS,
  });
  return true;
}

bool SpaNetComponent::request_light_color(uint16_t hue_degrees) {
  uint8_t color_index = this->hue_to_color_index_(hue_degrees);
  const std::string color_index_str = std::to_string(color_index);
  this->enqueue_command_(QueuedCommand{
      .kind = CommandKind::kLightColor,
      .payload = "S10:" + color_index_str,
      .expected_ack = color_index_str,
      .timeout_ms = COMMAND_TIMEOUT_MS,
  });
  return true;
}

bool SpaNetComponent::request_light_effect_mode(uint8_t mode) {
  if (mode > 4) {
    ESP_LOGW(TAG, "Rejected invalid light effect mode %u (max 4)", mode);
    return false;
  }
  const std::string mode_str = std::to_string(mode);
  this->enqueue_command_(QueuedCommand{
      .kind = CommandKind::kLightEffectMode,
      .payload = "S07:" + mode_str,
      .expected_ack = mode_str,
      .timeout_ms = COMMAND_TIMEOUT_MS,
  });
  return true;
}

bool SpaNetComponent::request_light_effect_speed(uint8_t speed) {
  if (speed < 1 || speed > 5) {
    ESP_LOGW(TAG, "Rejected invalid light effect speed %u (range 1-5)", speed);
    return false;
  }
  const std::string speed_str = std::to_string(speed);
  this->enqueue_command_(QueuedCommand{
      .kind = CommandKind::kLightEffectSpeed,
      .payload = "S09:" + speed_str,
      .expected_ack = speed_str,
      .timeout_ms = COMMAND_TIMEOUT_MS,
  });
  return true;
}

}  // namespace esphome::spanet
