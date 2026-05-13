#pragma once

#include "esphome/components/light/light_output.h"
#include "esphome/core/component.h"

#include "spanet.h"

namespace esphome::spanet {

class SpaNetLight : public light::LightOutput, public Component {
 public:
  // Light color map: maps hue index (0-24) to device color index (0-31)
  // colorMap[i] is the device color for hue = i * 15°
  static constexpr std::array<uint8_t, 25> LIGHT_COLOR_MAP{
      {0, 4, 4, 19, 13, 25, 25, 16, 10, 7, 2, 8, 5, 3, 6, 6, 21, 21, 21, 18, 18, 9, 9, 1, 1}};

  SpaNetLight(SpaNetComponent *parent) : parent_(parent) {}

  void setup() override;
  void dump_config() override;
  void setup_state(light::LightState *state) override;

 protected:
  light::LightTraits get_traits() override;
  void write_state(light::LightState *state) override;

 private:
  void handle_state_update_(const State &state);
  bool request_light_toggle_(bool desired_state);
  bool request_light_brightness_(uint8_t esphome_brightness);
  bool request_light_color_(uint16_t hue_degrees);
  static uint8_t esphome_brightness_to_device_(uint8_t esphome_0_255);
  static uint8_t device_brightness_to_esphome_(uint8_t device_1_5);
  static uint8_t hue_to_color_index_(uint16_t hue_degrees);
  static uint16_t color_index_to_hue_(uint8_t color_index);
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
