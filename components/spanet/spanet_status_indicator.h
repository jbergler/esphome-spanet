#pragma once

#include <cstdint>
#include <vector>

#include "esphome/components/light/light_state.h"
#include "esphome/components/output/binary_output.h"
#include "esphome/core/component.h"

namespace esphome::spanet {

class SpaNetComponent;

enum class StatusState : uint8_t {
  kNormal,
  kApMode,
  kWifiDisconnected,
  kSpaDisconnected,
  kApiDisconnected,
};

// Drives status LEDs based on WiFi/spa/API connection state.
//
// Three mutually-exclusive hardware modes (set exactly one via setters):
//   set_led()       — single binary output (DIY board)
//   add_led()       — up to 4 binary outputs (v1 knight-rider)
//   set_rgb_light() — single addressable RGB light (v2 NeoPixel)
class SpaNetStatusIndicator : public Component {
 public:
  explicit SpaNetStatusIndicator(SpaNetComponent *parent) : parent_(parent) {}

  void set_led(output::BinaryOutput *led) { this->single_led_ = led; }
  void add_led(output::BinaryOutput *led) { this->leds_.push_back(led); }
  void set_rgb_light(light::LightState *light) { this->rgb_light_ = light; }

  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::LATE; }

 protected:
  StatusState determine_state_() const;

  void tick_single_led_(StatusState state, uint32_t now_ms);
  void tick_multi_led_(StatusState state, uint32_t now_ms);
  void tick_rgb_(StatusState state);

  void all_leds_(bool on);
  void rgb_set_(uint8_t r, uint8_t g, uint8_t b);
  static uint8_t gamma8_(uint8_t v);

  SpaNetComponent *parent_;

  output::BinaryOutput *single_led_{nullptr};
  std::vector<output::BinaryOutput *> leds_{};
  light::LightState *rgb_light_{nullptr};

  static constexpr uint32_t TICK_MS = 20;
  uint32_t last_tick_ms_{0};

  // Single-LED state
  bool single_led_on_{false};
  uint32_t single_led_last_toggle_ms_{0};

  // Multi-LED state
  int8_t knight_pos_{0};
  int8_t knight_dir_{1};
  uint32_t knight_last_step_ms_{0};
  static constexpr uint32_t KNIGHT_STEP_MS = 150;

  // RGB state — 16-bit phase counter wraps naturally for smooth looping
  uint16_t anim_phase_{0};
  StatusState last_rgb_state_{StatusState::kNormal};
};

}  // namespace esphome::spanet
