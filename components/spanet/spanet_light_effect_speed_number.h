#pragma once

#include "esphome/components/number/number.h"
#include "esphome/core/component.h"

#include "spanet.h"

namespace esphome::spanet {

class SpaNetLightEffectSpeedNumber : public number::Number, public Component {
 public:
  explicit SpaNetLightEffectSpeedNumber(SpaNetComponent *parent) : parent_(parent) {}

  void setup() override;
  void dump_config() override;

 protected:
  void control(float value) override;

 private:
  void handle_state_update_(const State &state);
  bool request_light_effect_speed_(uint8_t speed);

  SpaNetComponent *parent_;
  bool has_published_state_{false};
  uint8_t last_speed_{0};
};

}  // namespace esphome::spanet
