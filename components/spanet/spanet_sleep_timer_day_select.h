#pragma once

#include "esphome/components/select/select.h"
#include "esphome/core/component.h"

#include "spanet.h"

namespace esphome::spanet {

class SpaNetSleepTimerDaySelect : public select::Select, public Component {
 public:
  SpaNetSleepTimerDaySelect(SpaNetComponent *parent, uint8_t timer_index)
      : parent_(parent), timer_index_(timer_index) {}

  void setup() override;
  void dump_config() override;

 protected:
  void control(const std::string &value) override;

 private:
  void handle_state_update_(const State &state);
  bool request_day_pattern_(int raw_value);

  SpaNetComponent *parent_;
  uint8_t timer_index_;  // 1 or 2
  bool has_published_state_{false};
  int last_raw_value_{-1};
};

}  // namespace esphome::spanet
