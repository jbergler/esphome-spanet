#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"

#include "command_queue.h"
#include "spanet_parser.h"
#include "spanet_state.h"
#include "update_debounce.h"
#include "uart_rx_buffer.h"

namespace esphome::spanet {

class SpaNetComponent : public PollingComponent, public uart::UARTDevice {
 public:
  using StateUpdateCallback = std::function<void(const State &)>;

  void set_controller_model_sensor(text_sensor::TextSensor *sensor) { this->sen_controller_model_ = sensor; }
  void set_controller_serial_sensor(text_sensor::TextSensor *sensor) { this->sen_controller_serial_ = sensor; }
  void set_controller_fw_version_sensor(text_sensor::TextSensor *sensor) {
    this->sen_controller_fw_version_ = sensor;
  }
  void set_water_temperature_sensor(sensor::Sensor *sensor) { this->sen_water_temperature_ = sensor; }
  void set_setpoint_temperature_sensor(sensor::Sensor *sensor) { this->sen_setpoint_temperature_ = sensor; }
  void set_heater_temperature_sensor(sensor::Sensor *sensor) { this->sen_heater_temperature_ = sensor; }
  void set_case_temperature_sensor(sensor::Sensor *sensor) { this->sen_case_temperature_ = sensor; }
  void set_mains_voltage_sensor(sensor::Sensor *sensor) { this->sen_mains_voltage_ = sensor; }
  void set_mains_current_sensor(sensor::Sensor *sensor) { this->sen_mains_current_ = sensor; }
  void set_instant_power_sensor(sensor::Sensor *sensor) { this->sen_instant_power_ = sensor; }
  void set_total_energy_sensor(sensor::Sensor *sensor) { this->sen_total_energy_ = sensor; }
  void add_on_state_callback(StateUpdateCallback callback) { this->state_callbacks_.push_back(std::move(callback)); }
  bool request_setpoint_temperature(float target_c);
  bool is_setpoint_write_pending() const;
  const State &get_state() const { return this->register_store_.get_state(); }

  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;

 protected:
  void on_uart_message_(const std::string &message);
  void on_state_update_message_(const std::string &message);
  void on_ack_message_(const std::string &message);
  void notify_state_update_(const State &state);
  std::optional<int> quantize_and_encode_setpoint_(float target_c);
  void enqueue_command_(QueuedCommand command);
  void process_command_timeouts_(uint32_t now_ms);
  virtual void send_uart_command_(const std::string &command);

  text_sensor::TextSensor *sen_controller_model_{nullptr};
  text_sensor::TextSensor *sen_controller_serial_{nullptr};
  text_sensor::TextSensor *sen_controller_fw_version_{nullptr};
  sensor::Sensor *sen_water_temperature_{nullptr};
  sensor::Sensor *sen_setpoint_temperature_{nullptr};
  sensor::Sensor *sen_heater_temperature_{nullptr};
  sensor::Sensor *sen_case_temperature_{nullptr};
  sensor::Sensor *sen_mains_voltage_{nullptr};
  sensor::Sensor *sen_mains_current_{nullptr};
  sensor::Sensor *sen_instant_power_{nullptr};
  sensor::Sensor *sen_total_energy_{nullptr};
  UartRxBuffer rx_buffer_{256};
  RegisterStore register_store_;
  UpdateDebounceGate state_update_debounce_{250};
  std::vector<StateUpdateCallback> state_callbacks_;
  std::unique_ptr<CommandQueue> command_queue_;
  std::optional<int> awaiting_setpoint_reconcile_tenths_;
};

}  // namespace esphome::spanet
