#pragma once

#include <cstdint>

#include "esphome/components/fan/fan.h"
#include "esphome/core/component.h"

#include "spanet.h"

namespace esphome::spanet {

class SpaNetPumpFan : public fan::Fan, public Component {
 public:
  SpaNetPumpFan(SpaNetComponent *parent, uint8_t pump_index) : parent_(parent), pump_index_(pump_index) {}

  void setup() override;
  void dump_config() override;

 protected:
  fan::FanTraits get_traits() override;
  void control(const fan::FanCall &call) override;

 private:
  void handle_state_update_(const State &state);
  int resolve_manual_speed_(const PumpStatus &pump) const;
  void sync_supported_presets_(const PumpStatus &pump);
  bool request_pump_mode_(int raw_mode);

  SpaNetComponent *parent_;
  uint8_t pump_index_;
  bool has_published_state_{false};
};

}  // namespace esphome::spanet
