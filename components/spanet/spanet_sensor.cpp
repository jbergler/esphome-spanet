#include "spanet_sensor.h"

#include <cmath>
#include <unordered_map>

#include "esphome/core/log.h"

namespace esphome::spanet {

static const char *const TAG = "spanet.sensor";

void SpaNetSensor::setup() {
  this->parent_->add_on_state_callback([this](const State &state) { this->handle_state_update_(state); });
  this->handle_state_update_(this->parent_->get_state());
}

void SpaNetSensor::dump_config() { LOG_SENSOR("", "SpaNET Sensor", this); }

using StateAccessor = std::optional<float> (*)(const State &);

static const std::unordered_map<SensorKind, StateAccessor> kAccessors = {
    {kWaterTemperature, [](const State &s) { return s.temperatures.water_c; }},
    {kSetpointTemperature, [](const State &s) { return s.temperatures.setpoint_c; }},
    {kHeaterTemperature, [](const State &s) { return s.temperatures.heater_c; }},
    {kCaseTemperature, [](const State &s) { return s.temperatures.case_c; }},
    {kMainsVoltage, [](const State &s) { return s.power.mains_voltage_v; }},
    {kMainsCurrent, [](const State &s) { return s.power.mains_current_a; }},
    {kInstantPower, [](const State &s) { return s.power.instant_power_w; }},
    {kTotalEnergy, [](const State &s) { return s.power.total_energy_kwh; }},
};

void SpaNetSensor::handle_state_update_(const State &state) {
  const auto it = kAccessors.find(this->kind_);
  if (it == kAccessors.end()) {
    return;
  }
  const auto value = it->second(state);
  if (value.has_value()) {
    const float next = value.value();
    if (std::isnan(this->state) || this->state != next) {
      this->publish_state(next);
    }
  }
}

}  // namespace esphome::spanet
