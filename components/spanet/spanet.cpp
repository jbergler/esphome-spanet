#include "spanet.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome::spanet {

static const char *const TAG = "spanet";
static constexpr const char *const STATE_UPDATE_DEBOUNCE_TIMEOUT = "state_update_debounce";
static constexpr uint32_t STATE_UPDATE_DEBOUNCE_MS = 250;
static constexpr size_t MAX_QUEUED_COMMANDS = 4;

void SpaNetComponent::setup() {
  ESP_LOGI(TAG, "Configuring SpaNET component");
  this->check_uart_settings(38400);

  this->command_queue_ =
      std::make_unique<CommandQueue>([this](const std::string &command) { this->send_uart_command_(command); },
                                     []() { return millis(); }, MAX_QUEUED_COMMANDS);

  this->add_on_state_callback([this](const State &state) {
    const auto &controller = state.controller_status;
    const auto &temperatures = state.temperatures;
    const auto &power = state.power;

    // Controller information
    this->publish_text_sensor_if_changed_(this->sen_controller_model_, controller.model);
    this->publish_text_sensor_if_changed_(this->sen_controller_fw_version_, controller.software_version);
    this->publish_text_sensor_if_changed_(this->sen_controller_serial_, controller.serial_number);

    // Temperatures
    this->publish_float_sensor_if_changed_(this->sen_water_temperature_, temperatures.water_c);
    this->publish_float_sensor_if_changed_(this->sen_setpoint_temperature_, temperatures.setpoint_c);
    this->publish_float_sensor_if_changed_(this->sen_heater_temperature_, temperatures.heater_c);
    this->publish_float_sensor_if_changed_(this->sen_case_temperature_, temperatures.case_c);

    // Power
    this->publish_float_sensor_if_changed_(this->sen_mains_voltage_, power.mains_voltage_v);
    this->publish_float_sensor_if_changed_(this->sen_mains_current_, power.mains_current_a);
    this->publish_float_sensor_if_changed_(this->sen_instant_power_, power.instant_power_w);
    this->publish_float_sensor_if_changed_(this->sen_total_energy_, power.total_energy_kwh);
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
      if (matched_command.command.triggers_rf_poll) {
        this->enqueue_command_(Command{
            .kind = CommandKind::kRfPoll,
            .payload = "RF",
            .expected_ack = "RF:",
            .timeout_ms = 500,
            .triggers_rf_poll = false,
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
  this->enqueue_command_(Command{
      .kind = CommandKind::kRfPoll,
      .payload = "RF",
      .expected_ack = "RF:",
      .timeout_ms = 500,
      .triggers_rf_poll = false,
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

void SpaNetComponent::enqueue_command_(Command command) {
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

  ESP_LOGW(TAG, "Command timed out after %u ms: payload=%s", age_ms, timed_out_command.command.payload.c_str());
}

void SpaNetComponent::send_uart_command_(const std::string &command) {
  ESP_LOGI(TAG, "UART TX: %s", command.c_str());
  this->write_str(command.c_str());
}

}  // namespace esphome::spanet
