#pragma once

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"

#include "spanet_parser.h"
#include "spanet_state.h"
#include "uart_rx_buffer.h"

namespace esphome::spanet {

class SpaNetComponent : public PollingComponent, public uart::UARTDevice {
 public:
  void set_controller_sensor(text_sensor::TextSensor *sensor) { this->controller_sensor_ = sensor; }

  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;

 protected:
  void on_uart_message_(const std::string &message);
  void on_state_update_message_(const std::string &message);
  void on_ack_message_(const std::string &message);
  bool should_recompute_state_() const;
  void recompute_state_();

  text_sensor::TextSensor *controller_sensor_{nullptr};
  UartRxBuffer rx_buffer_{256};
  RfRegisterStore register_store_;
  SpaNetState state_;
  bool state_dirty_{false};
  uint32_t last_rx_data_ms_{0};
};

}  // namespace esphome::spanet
