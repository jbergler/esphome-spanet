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

  // TODO: Hand complete messages to parser once parser is implemented.
  // this->parser_.parse(message);
}

void SpaNetComponent::update() {
  // Dummy hardcoded model value from component runtime logic.
  // In later steps this will come from parsed controller data.
  if (this->controller_sensor_ != nullptr) {
    this->controller_sensor_->publish_state("SVM1");
  }
}

void SpaNetComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "SpaNET dummy component");
  LOG_UPDATE_INTERVAL(this);
}

}  // namespace esphome::spanet
