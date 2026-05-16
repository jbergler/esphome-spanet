#include "spanet_light_effect_speed_number.h"

#include <cmath>

#include "esphome/core/log.h"

namespace esphome::spanet {

static const char *const TAG = "spanet.light_speed";
static constexpr uint32_t COMMAND_TIMEOUT_MS = 1000;

void SpaNetLightEffectSpeedNumber::setup() {
  this->parent_->add_on_state_callback([this](const State &state) { this->handle_state_update_(state); });
  this->handle_state_update_(this->parent_->get_state());
}

void SpaNetLightEffectSpeedNumber::dump_config() { LOG_NUMBER("", "SpaNET Light Effect Speed", this); }

void SpaNetLightEffectSpeedNumber::control(float value) {
  int rounded = static_cast<int>(std::lround(value));
  if (rounded < 1) {
    rounded = 1;
  } else if (rounded > 5) {
    rounded = 5;
  }

  if (!this->request_light_effect_speed_(static_cast<uint8_t>(rounded))) {
    ESP_LOGW(TAG, "Light effect speed request rejected");
    return;
  }
}

void SpaNetLightEffectSpeedNumber::handle_state_update_(const State &state) {
  if (this->has_published_state_ && this->last_speed_ == state.light.effect_speed) {
    return;
  }

  this->publish_state(static_cast<float>(state.light.effect_speed));
  this->has_published_state_ = true;
  this->last_speed_ = state.light.effect_speed;
}

bool SpaNetLightEffectSpeedNumber::request_light_effect_speed_(uint8_t speed) {
  if (speed < 1 || speed > 5) {
    ESP_LOGW(TAG, "Rejected invalid light effect speed %u (range 1-5)", speed);
    return false;
  }
  const std::string speed_str = std::to_string(speed);
  this->parent_->enqueue_command_(Command{
      .kind = CommandKind::kLightEffectSpeed,
      .payload = "S09:" + speed_str,
      .expected_acks = {speed_str},
      .timeout_ms = COMMAND_TIMEOUT_MS,
      .triggers_rf_poll = true,
  });
  return true;
}

}  // namespace esphome::spanet
