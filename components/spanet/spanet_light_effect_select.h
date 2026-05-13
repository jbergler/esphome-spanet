#pragma once

#include "esphome/components/select/select.h"
#include "esphome/core/component.h"

#include "spanet.h"

namespace esphome::spanet {

class SpaNetLightEffectSelect : public select::Select, public Component {
 public:
  explicit SpaNetLightEffectSelect(SpaNetComponent *parent) : parent_(parent) {}

  void setup() override;
  void dump_config() override;

 protected:
  void control(const std::string &value) override;

 private:
  void handle_state_update_(const State &state);
  bool request_light_effect_mode_(uint8_t mode);

  SpaNetComponent *parent_;
  bool has_published_state_{false};
  uint8_t last_effect_mode_{0};
};

}  // namespace esphome::spanet
