#pragma once

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"

namespace esphome::spanet {

class SpaNetComponent : public PollingComponent {
 public:
  void set_controller_sensor(text_sensor::TextSensor *sensor) { this->controller_sensor_ = sensor; }

  void setup() override;
  void update() override;
  void dump_config() override;

 protected:
  text_sensor::TextSensor *controller_sensor_{nullptr};
};

}  // namespace esphome::spanet
