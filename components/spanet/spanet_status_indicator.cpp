#include "spanet_status_indicator.h"

#include "spanet.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#ifdef USE_WIFI
#include "esphome/components/wifi/wifi_component.h"
#endif
#ifdef USE_API
#include "esphome/components/api/api_server.h"
#endif

#include <cmath>
#include <cstdlib>

namespace esphome::spanet {

static const char *const TAG = "spanet.status";

// ─── setup ────────────────────────────────────────────────────────────────────

void SpaNetStatusIndicator::setup() {
  if (this->single_led_ != nullptr) {
    this->single_led_->set_state(false);
  }
  this->all_leds_(false);
  if (this->rgb_light_ != nullptr) {
    auto call = this->rgb_light_->make_call();
    call.set_state(false);
    call.set_publish(false);
    call.set_save(false);
    call.perform();
  }
}

// ─── loop ─────────────────────────────────────────────────────────────────────

void SpaNetStatusIndicator::loop() {
  const uint32_t now_ms = millis();
  if (now_ms - this->last_tick_ms_ < TICK_MS)
    return;
  this->last_tick_ms_ = now_ms;

  const StatusState state = this->determine_state_();

  if (this->single_led_ != nullptr) {
    tick_single_led_(state, now_ms);
  } else if (!this->leds_.empty()) {
    tick_multi_led_(state, now_ms);
  } else if (this->rgb_light_ != nullptr) {
    tick_rgb_(state);
  }
}

// ─── state determination ──────────────────────────────────────────────────────

StatusState SpaNetStatusIndicator::determine_state_() const {
#ifdef USE_WIFI
  if (wifi::global_wifi_component != nullptr) {
    if (!wifi::global_wifi_component->is_connected()) {
      if (wifi::global_wifi_component->is_ap_active()) {
        return StatusState::kApMode;
      }
      return StatusState::kWifiDisconnected;
    }
  }
#endif

  if (!this->parent_->is_spa_data_fresh()) {
    return StatusState::kSpaDisconnected;
  }

#ifdef USE_API
  if (api::global_api_server != nullptr && !api::global_api_server->is_connected()) {
    return StatusState::kApiDisconnected;
  }
#endif

  return StatusState::kNormal;
}

// ─── single LED ───────────────────────────────────────────────────────────────

void SpaNetStatusIndicator::tick_single_led_(StatusState state, uint32_t now_ms) {
  if (state == StatusState::kApMode) {
    this->single_led_->set_state(true);
    return;
  }
  if (state == StatusState::kNormal) {
    this->single_led_->set_state(false);
    return;
  }

  uint32_t interval_ms;
  switch (state) {
    case StatusState::kWifiDisconnected:
      interval_ms = 100;
      break;
    case StatusState::kApiDisconnected:
      interval_ms = 500;
      break;
    case StatusState::kSpaDisconnected:
      interval_ms = 1000;
      break;
    default:
      interval_ms = 500;
      break;
  }

  if (now_ms - this->single_led_last_toggle_ms_ >= interval_ms) {
    this->single_led_on_ = !this->single_led_on_;
    this->single_led_->set_state(this->single_led_on_);
    this->single_led_last_toggle_ms_ = now_ms;
  }
}

// ─── multi-LED ────────────────────────────────────────────────────────────────

void SpaNetStatusIndicator::all_leds_(bool on) {
  for (auto *led : this->leds_) {
    led->set_state(on);
  }
}

void SpaNetStatusIndicator::tick_multi_led_(StatusState state, uint32_t now_ms) {
  if (state == StatusState::kApMode) {
    this->all_leds_(true);
    return;
  }

  if (state != StatusState::kNormal) {
    // Error: light exactly one LED. Assignment is relative to list end so it
    // works regardless of how many LEDs are configured:
    //   last        → WiFi disconnected
    //   second-last → spa disconnected
    //   third-last  → API disconnected
    const size_t n = this->leds_.size();
    size_t idx;
    switch (state) {
      case StatusState::kWifiDisconnected:
        idx = n - 1;
        break;
      case StatusState::kSpaDisconnected:
        idx = n >= 2 ? n - 2 : n - 1;
        break;
      case StatusState::kApiDisconnected:
        idx = n >= 3 ? n - 3 : 0;
        break;
      default:
        idx = 0;
        break;
    }
    for (size_t i = 0; i < n; i++) {
      this->leds_[i]->set_state(i == idx);
    }
    return;
  }

  // Normal: knight rider sweep (150 ms per step)
  if (now_ms - this->knight_last_step_ms_ < KNIGHT_STEP_MS)
    return;
  this->knight_last_step_ms_ = now_ms;

  const int8_t n = static_cast<int8_t>(this->leds_.size());
  for (int8_t i = 0; i < n; i++) {
    this->leds_[i]->set_state(i == this->knight_pos_);
  }
  this->knight_pos_ += this->knight_dir_;
  if (this->knight_pos_ >= n - 1 || this->knight_pos_ <= 0) {
    this->knight_dir_ = -this->knight_dir_;
  }
}

// ─── RGB ──────────────────────────────────────────────────────────────────────

void SpaNetStatusIndicator::rgb_set_(uint8_t r, uint8_t g, uint8_t b) {
  if (this->rgb_light_ == nullptr)
    return;
  auto call = this->rgb_light_->make_call();
  call.set_publish(false);
  call.set_save(false);
  call.set_transition_length(0);
  if (r == 0 && g == 0 && b == 0) {
    call.set_state(false);
  } else {
    // set_brightness(1.0) ensures the (r,g,b) values map directly to pixel
    // output without additional brightness scaling. ESPHome's gamma correction
    // is applied to these linear values by the light driver before the hardware.
    call.set_state(true);
    call.set_rgb(r / 255.0f, g / 255.0f, b / 255.0f);
    call.set_brightness(1.0f);
  }
  call.perform();
}

void SpaNetStatusIndicator::tick_rgb_(StatusState state) {
  // Reset animation phase on state transitions for clean entry.
  if (state != this->last_rgb_state_) {
    this->anim_phase_ = 0;
    this->last_rgb_state_ = state;
  }

  switch (state) {
    case StatusState::kNormal: {
      // Rainbow cycle.
      // HUE_STEP=128 → 65536/128 ticks × 20 ms ≈ 10.2 s per cycle.
      this->anim_phase_ += 128;
      const uint8_t hue = static_cast<uint8_t>(this->anim_phase_ >> 8);

      // Inline HSV→RGB: sat=255, val=150 (linear value; ESPHome gamma maps this
      // to comfortable brightness on the physical pixel).
      static constexpr uint8_t SAT = 255, VAL = 150;
      const uint8_t region = hue / 43;
      const uint8_t rem = (hue - region * 43) * 6;
      const uint8_t p = static_cast<uint8_t>((VAL * (255 - SAT)) >> 8);
      const uint8_t q = static_cast<uint8_t>((VAL * (255 - ((SAT * rem) >> 8))) >> 8);
      const uint8_t t = static_cast<uint8_t>((VAL * (255 - ((SAT * (255 - rem)) >> 8))) >> 8);
      switch (region) {
        case 0:
          rgb_set_(VAL, t, p);
          break;
        case 1:
          rgb_set_(q, VAL, p);
          break;
        case 2:
          rgb_set_(p, VAL, t);
          break;
        case 3:
          rgb_set_(p, q, VAL);
          break;
        case 4:
          rgb_set_(t, p, VAL);
          break;
        default:
          rgb_set_(VAL, p, q);
          break;
      }
      break;
    }

    case StatusState::kApMode: {
      // Blue ↔ cyan colour blend, sinusoidal for smooth transitions.
      // += 437 → 65536/437 ≈ 150 ticks × 20 ms ≈ 3 s cycle.
      this->anim_phase_ += 437;
      const float t = this->anim_phase_ * (1.0f / 65536.0f);
      const float g_frac = (1.0f - std::cosf(2.0f * static_cast<float>(M_PI) * t)) * 0.5f;
      // {0, 0, 150} ↔ {0, 150, 150}; G computed in float to preserve hue at low brightness.
      rgb_set_(0, static_cast<uint8_t>(g_frac * 150.0f), 150);
      break;
    }

    case StatusState::kWifiDisconnected: {
      // Red heartbeat: two quick pulses then a long pause.
      // += 875 → 65536/875 ≈ 75 ticks × 20 ms ≈ 1.5 s cycle.
      this->anim_phase_ += 875;
      uint8_t brightness;
      if (this->anim_phase_ < 8192u) {
        brightness = static_cast<uint8_t>(this->anim_phase_ >> 5);  // pulse 1 up
      } else if (this->anim_phase_ < 16384u) {
        brightness = static_cast<uint8_t>((16383u - this->anim_phase_) >> 5);  // pulse 1 down
      } else if (this->anim_phase_ < 24576u) {
        brightness = static_cast<uint8_t>((this->anim_phase_ - 16384u) >> 5);  // pulse 2 up
      } else if (this->anim_phase_ < 32768u) {
        brightness = static_cast<uint8_t>((32767u - this->anim_phase_) >> 5);  // pulse 2 down
      } else {
        brightness = 0;  // pause
      }
      rgb_set_(brightness, 0, 0);
      break;
    }

    case StatusState::kSpaDisconnected: {
      // Yellow breathing: fixed hue {255,180,0}, only brightness varies.
      // Using set_brightness() keeps ESPHome's per-channel gamma scaling
      // proportional, so the yellow hue stays constant at all levels.
      // += 655 → 65536/655 ≈ 100 ticks × 20 ms ≈ 2 s per cycle.
      // sin³(πt): perceived brightness ∝ sin^3.8(πt) — ~62% of cycle dim.
      this->anim_phase_ += 655;
      const float t = this->anim_phase_ * (1.0f / 65536.0f);
      const float s = std::sinf(static_cast<float>(M_PI) * t);
      const float frac = s * s * s;
      {
        auto call = this->rgb_light_->make_call();
        call.set_publish(false);
        call.set_save(false);
        call.set_transition_length(0);
        if (frac < 0.001f) {
          call.set_state(false);
        } else {
          call.set_state(true);
          call.set_rgb(1.0f, 180.0f / 255.0f, 0.0f);
          call.set_brightness(frac);
        }
        call.perform();
      }
      break;
    }

    case StatusState::kApiDisconnected: {
      // Purple sparkle: dim base glow with occasional bright spikes.
      // Base values are linear inputs; ESPHome's gamma maps them to dim-but-visible output.
      static constexpr uint8_t BASE_R = 100, BASE_B = 150;
      uint8_t r, bv;
      if ((rand() % 100) < 15) {
        const uint8_t spike = static_cast<uint8_t>(rand() % 80);
        r = static_cast<uint8_t>(BASE_R + spike / 2);
        bv = static_cast<uint8_t>(BASE_B + spike);
      } else {
        r = static_cast<uint8_t>(BASE_R + rand() % 25);
        bv = static_cast<uint8_t>(BASE_B + rand() % 40);
      }
      rgb_set_(r, 0, bv);
      break;
    }
  }
}

}  // namespace esphome::spanet
