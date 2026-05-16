#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/core/component.h"

#include "spanet.h"

namespace esphome::spanet {

enum BinarySensorKind {
  kHeatingActive,
  kOzoneActive,
  kCleanCycleActive,
  kWaterPresent,
};

class SpaNetBinarySensor : public binary_sensor::BinarySensor, public Component {
 public:
  SpaNetBinarySensor(SpaNetComponent *parent, BinarySensorKind kind) : parent_(parent), kind_(kind) {}

  void setup() override;
  void dump_config() override;

 private:
  void handle_state_update_(const State &state);

  SpaNetComponent *parent_;
  BinarySensorKind kind_;
};

}  // namespace esphome::spanet
