#include "spanet_binary_sensor.h"

#include <unordered_map>

#include "esphome/core/log.h"

namespace esphome::spanet {

static const char *const TAG = "spanet.binary_sensor";

void SpaNetBinarySensor::setup() {
  this->parent_->add_on_state_callback([this](const State &state) { this->handle_state_update_(state); });
  this->handle_state_update_(this->parent_->get_state());
}

void SpaNetBinarySensor::dump_config() { LOG_BINARY_SENSOR("", "SpaNET Binary Sensor", this); }

using StateAccessor = std::optional<bool> (*)(const State &);

static const std::unordered_map<BinarySensorKind, StateAccessor> kAccessors = {
    {kHeatingActive, [](const State &s) { return s.climate.heating_active; }},
    {kOzoneActive, [](const State &s) { return s.spa_operating.ozone_active; }},
    {kCleanCycleActive, [](const State &s) { return s.spa_operating.clean_cycle_active; }},
    {kWaterPresent, [](const State &s) { return s.spa_operating.water_present; }},
};

void SpaNetBinarySensor::handle_state_update_(const State &state) {
  const auto it = kAccessors.find(this->kind_);
  if (it == kAccessors.end()) {
    return;
  }
  const auto value = it->second(state);
  if (value.has_value()) {
    const bool next = value.value();
    if (!this->has_state() || this->state != next) {
      this->publish_state(next);
    }
  }
}

}  // namespace esphome::spanet
