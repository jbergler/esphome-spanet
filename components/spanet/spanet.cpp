#include "spanet.h"
#include "esphome/core/log.h"

namespace esphome::spanet {

static const char *const TAG = "spanet";

void SpaNetComponent::setup() {
  ESP_LOGI(TAG, "Setting up dummy SpaNET component");
  this->check_uart_settings(38400);
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
    ESP_LOGW(TAG, "Failed to parse SpaNET state update payload");
    return;
  }
  ESP_LOGD(TAG, "Updated register store");
}

void SpaNetComponent::on_ack_message_(const std::string &message) {
  // TODO: Match acknowledgements against pending commands and invoke completion callbacks.
  ESP_LOGI(TAG, "Received command acknowledgement: %s", message.c_str());
}

void SpaNetComponent::update() {
  auto identity = this->register_store_.controller_identity();
  if (this->controller_sensor_ != nullptr && identity.has_value()) {
    this->controller_sensor_->publish_state(identity->model);
  }
}

void SpaNetComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "SpaNET dummy component");
  LOG_UPDATE_INTERVAL(this);
}

}  // namespace esphome::spanet
