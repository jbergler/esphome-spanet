#pragma once

#include "esphome/components/light/light_output.h"
#include "esphome/core/component.h"

#include "spanet.h"

namespace esphome::spanet {

class SpaNetLight : public light::LightOutput, public Component {
 public:
  SpaNetLight(SpaNetComponent *parent) : parent_(parent) {}

  void setup() override;
  void dump_config() override;
  void setup_state(light::LightState *state) override;

 protected:
  light::LightTraits get_traits() override;
  void write_state(light::LightState *state) override;

 private:
  void handle_state_update_(const State &state);
  static bool rgb_to_hue_(float red, float green, float blue, uint16_t *hue_degrees);
  static void hsv_to_rgb_(uint16_t hue_degrees, float saturation, float *red, float *green, float *blue);

  SpaNetComponent *parent_;
  light::LightState *state_{nullptr};
  bool applying_remote_update_{false};
  bool has_polled_state_{false};
  bool polled_on_{false};
  uint8_t polled_brightness_{0};
  uint16_t polled_hue_{0};
  bool has_published_state_{false};
  bool published_on_{false};
  uint8_t published_brightness_{0};
  uint16_t published_hue_{0};
  float published_saturation_{0.0f};
};

}  // namespace esphome::spanet
