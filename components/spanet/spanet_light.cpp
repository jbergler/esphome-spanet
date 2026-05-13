#include "spanet_light.h"

#include "esphome/core/log.h"

#include <algorithm>
#include <cmath>

namespace esphome::spanet {

static const char *const TAG = "spanet.light";

void SpaNetLight::setup() {
  this->parent_->add_on_state_callback([this](const State &state) { this->handle_state_update_(state); });
  this->handle_state_update_(this->parent_->get_state());
}

void SpaNetLight::dump_config() { ESP_LOGCONFIG(TAG, "SpaNET Light"); }

void SpaNetLight::setup_state(light::LightState *state) { this->state_ = state; }

light::LightTraits SpaNetLight::get_traits() {
  light::LightTraits traits;
  traits.set_supported_color_modes({light::ColorMode::RGB});
  return traits;
}

void SpaNetLight::write_state(light::LightState *state) {
  if (this->applying_remote_update_) {
    return;
  }

  const auto &values = state->remote_values;
  const bool desired_on = values.is_on();
  float desired_brightness_f = values.get_brightness();
  if (desired_brightness_f < 0.0f) {
    desired_brightness_f = 0.0f;
  } else if (desired_brightness_f > 1.0f) {
    desired_brightness_f = 1.0f;
  }
  const uint8_t desired_brightness = static_cast<uint8_t>(std::lround(desired_brightness_f * 255.0f));

  uint16_t desired_hue = this->polled_hue_;
  bool has_desired_hue = false;
  if (desired_on) {
    has_desired_hue = rgb_to_hue_(values.get_red(), values.get_green(), values.get_blue(), &desired_hue);
  }

  if (!this->has_polled_state_ || desired_on != this->polled_on_) {
    if (!this->parent_->request_light_toggle(desired_on)) {
      ESP_LOGW(TAG, "Light toggle request rejected by hub");
      return;
    }
  }

  if (desired_on && (!this->has_polled_state_ || desired_brightness != this->polled_brightness_)) {
    if (!this->parent_->request_light_brightness(desired_brightness)) {
      ESP_LOGW(TAG, "Light brightness request rejected by hub");
      return;
    }
  }

  if (desired_on && has_desired_hue && (!this->has_polled_state_ || desired_hue != this->polled_hue_)) {
    if (!this->parent_->request_light_color(desired_hue)) {
      ESP_LOGW(TAG, "Light color request rejected by hub");
      return;
    }
  }
}

void SpaNetLight::handle_state_update_(const State &state) {
  if (this->state_ == nullptr) {
    return;
  }

  const bool is_on = state.light.is_on;

  const uint8_t brightness_u8 = this->parent_->device_brightness_to_esphome_(state.light.brightness);
  const float brightness = brightness_u8 / 255.0f;
  const uint16_t hue = this->parent_->color_index_to_hue_(state.light.color_index);
  const float saturation = (state.light.effect_mode == 0) ? 0.0f : 1.0f;

  this->has_polled_state_ = true;
  this->polled_on_ = is_on;
  this->polled_brightness_ = brightness_u8;
  this->polled_hue_ = hue;

  if (this->has_published_state_ && this->published_on_ == is_on && this->published_brightness_ == brightness_u8 &&
      this->published_hue_ == hue && this->published_saturation_ == saturation) {
    return;
  }

  float red = 1.0f;
  float green = 1.0f;
  float blue = 1.0f;
  hsv_to_rgb_(hue, saturation, &red, &green, &blue);

  this->applying_remote_update_ = true;
  auto call = this->state_->make_call();
  call.set_state(is_on);
  call.set_brightness_if_supported(brightness);
  call.set_color_mode_if_supported(light::ColorMode::RGB);
  call.set_rgb(red, green, blue);
  call.perform();
  this->applying_remote_update_ = false;

  this->has_published_state_ = true;
  this->published_on_ = is_on;
  this->published_brightness_ = brightness_u8;
  this->published_hue_ = hue;
  this->published_saturation_ = saturation;
}

bool SpaNetLight::rgb_to_hue_(float red, float green, float blue, uint16_t *hue_degrees) {
  const float max_v = std::max({red, green, blue});
  const float min_v = std::min({red, green, blue});
  const float delta = max_v - min_v;

  if (delta <= 0.001f) {
    return false;
  }

  float hue = 0.0f;
  if (max_v == red) {
    hue = 60.0f * std::fmod(((green - blue) / delta), 6.0f);
  } else if (max_v == green) {
    hue = 60.0f * (((blue - red) / delta) + 2.0f);
  } else {
    hue = 60.0f * (((red - green) / delta) + 4.0f);
  }

  if (hue < 0.0f) {
    hue += 360.0f;
  }

  *hue_degrees = static_cast<uint16_t>(std::lround(hue)) % 360;
  return true;
}

void SpaNetLight::hsv_to_rgb_(uint16_t hue_degrees, float saturation, float *red, float *green, float *blue) {
  float sat = saturation;
  if (sat < 0.0f) {
    sat = 0.0f;
  } else if (sat > 1.0f) {
    sat = 1.0f;
  }
  if (sat <= 0.001f) {
    *red = 1.0f;
    *green = 1.0f;
    *blue = 1.0f;
    return;
  }

  const float hue = static_cast<float>(hue_degrees % 360);
  const float c = sat;
  const float x = c * (1.0f - std::fabs(std::fmod(hue / 60.0f, 2.0f) - 1.0f));

  float r1 = 0.0f;
  float g1 = 0.0f;
  float b1 = 0.0f;

  if (hue < 60.0f) {
    r1 = c;
    g1 = x;
  } else if (hue < 120.0f) {
    r1 = x;
    g1 = c;
  } else if (hue < 180.0f) {
    g1 = c;
    b1 = x;
  } else if (hue < 240.0f) {
    g1 = x;
    b1 = c;
  } else if (hue < 300.0f) {
    r1 = x;
    b1 = c;
  } else {
    r1 = c;
    b1 = x;
  }

  const float m = 1.0f - c;
  *red = r1 + m;
  *green = g1 + m;
  *blue = b1 + m;
}

}  // namespace esphome::spanet
