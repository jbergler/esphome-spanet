#pragma once

#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"

#include "spanet.h"
#include "spanet_time_sync.h"

namespace esphome::spanet {

enum TextSensorKind {
  kControllerModel,
  kControllerSerial,
  kControllerFwVersion,
  kCurrentTime,
};

class SpaNetTextSensor : public text_sensor::TextSensor, public Component {
 public:
  SpaNetTextSensor(SpaNetComponent *parent, TextSensorKind kind) : parent_(parent), kind_(kind) {}

  void setup() override;
  void dump_config() override;

 private:
  void handle_state_update_(const State &state);

  SpaNetComponent *parent_;
  TextSensorKind kind_;
};

}  // namespace esphome::spanet
