#pragma once

#include <functional>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"

#include "spanet_parser.h"
#include "spanet_state.h"
#include "uart_rx_buffer.h"

namespace esphome::spanet {

class SpaNetComponent : public PollingComponent, public uart::UARTDevice {
 public:
  using StateUpdateCallback = std::function<void(const State &)>;

  void set_model_sensor(text_sensor::TextSensor *sensor) { this->sen_model_ = sensor; }
  void add_on_state_callback(StateUpdateCallback callback) { this->state_callbacks_.push_back(std::move(callback)); }

  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;

 protected:
  void on_uart_message_(const std::string &message);
  void on_state_update_message_(const std::string &message);
  void on_ack_message_(const std::string &message);
  void notify_state_update_(const State &state);

  text_sensor::TextSensor *sen_model_{nullptr};
  UartRxBuffer rx_buffer_{256};
  RegisterStore register_store_;
  std::vector<StateUpdateCallback> state_callbacks_;
};

}  // namespace esphome::spanet
