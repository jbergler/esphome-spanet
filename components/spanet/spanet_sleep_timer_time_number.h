#pragma once

#include "esphome/components/number/number.h"
#include "esphome/core/component.h"

#include "spanet.h"

namespace esphome::spanet {

enum SleepTimerEndpoint { kBegin, kEnd };
enum SleepTimerField { kHour, kMinute };

class SpaNetSleepTimerTimeNumber : public number::Number, public Component {
 public:
  SpaNetSleepTimerTimeNumber(SpaNetComponent *parent, uint8_t timer_index, SleepTimerEndpoint endpoint,
                             SleepTimerField field)
      : parent_(parent), timer_index_(timer_index), endpoint_(endpoint), field_(field) {}

  void setup() override;
  void dump_config() override;

 protected:
  void control(float value) override;

 private:
  void handle_state_update_(const State &state);
  bool request_time_(int new_wire_val);

  SpaNetComponent *parent_;
  uint8_t timer_index_;          // 1 or 2
  SleepTimerEndpoint endpoint_;  // kBegin or kEnd
  SleepTimerField field_;        // kHour or kMinute
  bool has_published_state_{false};
  int last_wire_val_{0};  // HH*256+MM cache
};

}  // namespace esphome::spanet
