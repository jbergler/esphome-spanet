#include "spanet.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome::spanet {

static const char *const TAG = "spanet";

void SpaNetComponent::setup() {
  ESP_LOGI(TAG, "Setting up dummy SpaNET component");
  this->check_uart_settings(38400);

  this->add_on_state_callback([this](const State &state) {
    const auto &controller = state.controller_status;

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
  });
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
  this->notify_state_update_(this->register_store_.get_state());
}

void SpaNetComponent::on_ack_message_(const std::string &message) {
  // TODO: Match acknowledgements against pending commands and invoke completion callbacks.
  ESP_LOGI(TAG, "Received command acknowledgement: %s", message.c_str());
}

void SpaNetComponent::update() {
  // Event-driven publishing is done from on_state_update_message_.
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
}

}  // namespace esphome::spanet
