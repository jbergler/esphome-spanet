#include "spanet.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome::spanet {

static const char *const TAG = "spanet";

void SpaNetComponent::setup() {
  ESP_LOGI(TAG, "Setting up dummy SpaNET component");
  this->check_uart_settings(38400);
}

void SpaNetComponent::loop() {
  bool saw_new_data = false;

  while (this->available() > 0) {
    uint8_t byte;
    if (!this->read_byte(&byte)) {
      break;
    }

    saw_new_data = true;
    this->last_rx_data_ms_ = millis();

    auto maybe_message = this->rx_buffer_.feed(byte);
    if (maybe_message.has_value()) {
      this->on_uart_message_(maybe_message.value());
    }
  }

  if (saw_new_data) {
    this->state_dirty_ = true;
  }

  if (this->state_dirty_ && this->should_recompute_state_()) {
    this->recompute_state_();
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
  auto parsed = SpaNetParser::parse_register_line(message);
  if (!parsed.has_value()) {
    ESP_LOGD(TAG, "State update line had no register payload; skipping");
    return;
  }

  if (!this->register_store_.update_fields(parsed->first, parsed->second)) {
    ESP_LOGW(TAG, "Failed to update register store for SpaNET state payload");
    return;
  }

  this->state_dirty_ = true;
  ESP_LOGD(TAG, "Updated register store");
}

void SpaNetComponent::on_ack_message_(const std::string &message) {
  // TODO: Match acknowledgements against pending commands and invoke completion callbacks.
  ESP_LOGI(TAG, "Received command acknowledgement: %s", message.c_str());
}

void SpaNetComponent::update() {
  if (this->controller_sensor_ != nullptr && !this->state_.controller.empty()) {
    this->controller_sensor_->publish_state(this->state_.controller.model);
  }
}

bool SpaNetComponent::should_recompute_state_() const {
  if (this->rx_buffer_.empty()) {
    return true;
  }

  return millis() - this->last_rx_data_ms_ >= 100;
}

void SpaNetComponent::recompute_state_() {
  if (!this->state_.recompute_from(this->register_store_)) {
    ESP_LOGW(TAG, "Failed to recompute SpaNetState from register store");
    return;
  }

  this->state_dirty_ = false;
  ESP_LOGD(TAG, "Recomputed SpaNetState");
}

void SpaNetComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "SpaNET dummy component");
  LOG_UPDATE_INTERVAL(this);
}

}  // namespace esphome::spanet
