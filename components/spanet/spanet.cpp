#include "spanet.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

#include <cmath>

namespace esphome::spanet {

static const char *const TAG = "spanet";
static constexpr const char *const STATE_UPDATE_DEBOUNCE_TIMEOUT = "state_update_debounce";
static constexpr uint32_t STATE_UPDATE_DEBOUNCE_MS = 250;

void SpaNetComponent::setup() {
  ESP_LOGI(TAG, "Setting up dummy SpaNET component");
  this->check_uart_settings(38400);

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

  // Trigger an initial poll immediately; PollingComponent handles recurring polls.
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
}

void SpaNetComponent::on_uart_message_(const std::string &message) {
  ESP_LOGI(TAG, "Received UART message: %s", message.c_str());

  switch (SpaNetParser::classify_message(message)) {
    case MessageType::kStateUpdate:
      this->on_state_update_message_(message);
      break;
    case MessageType::kAck:
      this->on_ack_message_(message);
      break;
    case MessageType::kUnknown:
      ESP_LOGW(TAG, "Ignoring unknown UART payload");
      break;
  }
}

void SpaNetComponent::on_state_update_message_(const std::string &message) {
  if (!this->register_store_.update(message)) {
    ESP_LOGW(TAG, "Failed to update register store for SpaNET state payload");
    return;
  }

  ESP_LOGD(TAG, "Updated register store");
  if (!this->state_update_debounce_.try_arm()) {
    return;
  }

  this->set_timeout(STATE_UPDATE_DEBOUNCE_TIMEOUT, STATE_UPDATE_DEBOUNCE_MS, [this]() {
    this->state_update_debounce_.disarm();
    this->notify_state_update_(this->register_store_.get_state());
  });
}

void SpaNetComponent::on_ack_message_(const std::string &message) {
  // TODO: Match acknowledgements against pending commands and invoke completion callbacks.
  ESP_LOGI(TAG, "Received command acknowledgement: %s", message.c_str());
}

void SpaNetComponent::update() {
  ESP_LOGV(TAG, "Requesting state poll: RF");
  this->write_str("RF\n");
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

}  // namespace esphome::spanet
