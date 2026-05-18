#pragma once

#include "esphome/components/number/number.h"
#include "esphome/core/component.h"

#include "spanet.h"

namespace esphome::spanet {

class SpaNetFiltHrsNumber : public number::Number, public Component {
 public:
  explicit SpaNetFiltHrsNumber(SpaNetComponent *parent) : parent_(parent) {}

  void setup() override;
  void dump_config() override;

 protected:
  void control(float value) override;

 private:
  void handle_state_update_(const State &state);
  bool request_filt_hrs_(int hrs);

  SpaNetComponent *parent_;
  bool has_published_state_{false};
  int last_hrs_{0};
};

}  // namespace esphome::spanet
