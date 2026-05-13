#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

#include "command_queue.h"
#include "spanet_parser.h"
#include "spanet_state.h"
#include "uart_rx_buffer.h"
#include "update_debounce.h"

namespace esphome::spanet {

class SpaNetComponent : public PollingComponent, public uart::UARTDevice {
 public:
  using StateUpdateCallback = std::function<void(const State &)>;

  // Light color map: maps hue index (0-24) to device color index (0-31)
  // colorMap[i] is the device color for hue = i * 15°
  static constexpr std::array<uint8_t, 25> LIGHT_COLOR_MAP{
      {0, 4, 4, 19, 13, 25, 25, 16, 10, 7, 2, 8, 5, 3, 6, 6, 21, 21, 21, 18, 18, 9, 9, 1, 1}};

  // Light effect mode strings
  static constexpr std::array<const char *, 5> LIGHT_EFFECT_MODES{{"White", "Colour", "Step", "Fade", "Party"}};

  void set_controller_model_sensor(text_sensor::TextSensor *sensor) { this->sen_controller_model_ = sensor; }
  void set_controller_serial_sensor(text_sensor::TextSensor *sensor) { this->sen_controller_serial_ = sensor; }
  void set_controller_fw_version_sensor(text_sensor::TextSensor *sensor) { this->sen_controller_fw_version_ = sensor; }
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
  bool request_pump_mode(uint8_t pump_index, int raw_mode);
  bool request_light_toggle(bool desired_state);
  bool request_light_brightness(uint8_t esphome_brightness);
  bool request_light_color(uint16_t hue_degrees);
  bool request_light_effect_mode(uint8_t mode);
  bool request_light_effect_speed(uint8_t speed);
  const State &get_state() const { return this->register_store_.get_state(); }

  // Public helper methods for light entity
  uint8_t device_brightness_to_esphome_(uint8_t device_1_5);
  uint16_t color_index_to_hue_(uint8_t color_index);

  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;

 protected:
  void on_uart_message_(const std::string &message);
  void on_state_update_message_(const std::string &message);
  void notify_state_update_(const State &state);
  std::optional<int> quantize_and_encode_setpoint_(float target_c);
  uint8_t esphome_brightness_to_device_(uint8_t esphome_0_255);
  uint8_t hue_to_color_index_(uint16_t hue_degrees);
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
  std::optional<bool> pending_light_toggle_state_;
};

}  // namespace esphome::spanet
