#pragma once

#include "esphome/components/select/select.h"
#include "esphome/core/component.h"

#include "spanet.h"

namespace esphome::spanet {

class SpaNetOperatingModeSelect : public select::Select, public Component {
 public:
  explicit SpaNetOperatingModeSelect(SpaNetComponent *parent) : parent_(parent) {}

  void setup() override;
  void dump_config() override;

 protected:
  void control(const std::string &value) override;

 private:
  void handle_state_update_(const State &state);
  bool request_operating_mode_(uint8_t mode);

  SpaNetComponent *parent_;
  bool has_published_state_{false};
  std::string last_mode_;
};

}  // namespace esphome::spanet
