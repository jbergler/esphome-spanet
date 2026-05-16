#pragma once

#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"

#include "spanet.h"

namespace esphome::spanet {

enum SensorKind {
  kWaterTemperature,
  kSetpointTemperature,
  kHeaterTemperature,
  kCaseTemperature,
  kMainsVoltage,
  kMainsCurrent,
  kInstantPower,
  kTotalEnergy,
};

class SpaNetSensor : public sensor::Sensor, public Component {
 public:
  SpaNetSensor(SpaNetComponent *parent, SensorKind kind) : parent_(parent), kind_(kind) {}

  void setup() override;
  void dump_config() override;

 private:
  void handle_state_update_(const State &state);

  SpaNetComponent *parent_;
  SensorKind kind_;
};

}  // namespace esphome::spanet
