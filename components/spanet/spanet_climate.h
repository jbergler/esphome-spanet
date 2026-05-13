#pragma once

#include "esphome/components/climate/climate.h"
#include "esphome/core/component.h"

#include "spanet.h"

namespace esphome::spanet {

class SpaNetClimate : public climate::Climate, public Component {
 public:
  explicit SpaNetClimate(SpaNetComponent *parent) : parent_(parent) {}

  void setup() override;
  void dump_config() override;

 protected:
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;

  void handle_state_update_(const State &state);
  bool request_setpoint_temperature_(float target_c);
  std::optional<int> quantize_and_encode_setpoint_(float target_c);

  SpaNetComponent *parent_;
};

}  // namespace esphome::spanet
