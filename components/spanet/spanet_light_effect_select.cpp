#include "spanet_light_effect_select.h"

#include "esphome/core/log.h"

namespace esphome::spanet {

static const char *const TAG = "spanet.light_effect";

void SpaNetLightEffectSelect::setup() {
  this->parent_->add_on_state_callback([this](const State &state) { this->handle_state_update_(state); });
  this->handle_state_update_(this->parent_->get_state());
}

void SpaNetLightEffectSelect::dump_config() { LOG_SELECT("", "SpaNET Light Effect", this); }

void SpaNetLightEffectSelect::control(const std::string &value) {
  uint8_t mode = 0;
  bool found = false;
  for (size_t i = 0; i < SpaNetComponent::LIGHT_EFFECT_MODES.size(); i++) {
    if (value == SpaNetComponent::LIGHT_EFFECT_MODES[i]) {
      mode = static_cast<uint8_t>(i);
      found = true;
      break;
    }
  }

  if (!found) {
    ESP_LOGW(TAG, "Unsupported light effect option '%s'", value.c_str());
    return;
  }

  if (!this->parent_->request_light_effect_mode(mode)) {
    ESP_LOGW(TAG, "Light effect request rejected by hub");
    return;
  }
}

void SpaNetLightEffectSelect::handle_state_update_(const State &state) {
  if (state.light.effect_mode >= SpaNetComponent::LIGHT_EFFECT_MODES.size()) {
    return;
  }

  if (this->has_published_state_ && this->last_effect_mode_ == state.light.effect_mode) {
    return;
  }

  this->publish_state(SpaNetComponent::LIGHT_EFFECT_MODES[state.light.effect_mode]);
  this->has_published_state_ = true;
  this->last_effect_mode_ = state.light.effect_mode;
}

}  // namespace esphome::spanet
