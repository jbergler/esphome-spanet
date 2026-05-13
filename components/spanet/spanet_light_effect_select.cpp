#include "spanet_light_effect_select.h"

#include "esphome/core/log.h"

namespace esphome::spanet {

static const char *const TAG = "spanet.light_effect";
static constexpr uint32_t COMMAND_TIMEOUT_MS = 1000;
static constexpr std::array<const char *, 5> LIGHT_EFFECT_MODES{{"White", "Colour", "Step", "Fade", "Party"}};

void SpaNetLightEffectSelect::setup() {
  this->parent_->add_on_state_callback([this](const State &state) { this->handle_state_update_(state); });
  this->handle_state_update_(this->parent_->get_state());
}

void SpaNetLightEffectSelect::dump_config() { LOG_SELECT("", "SpaNET Light Effect", this); }

void SpaNetLightEffectSelect::control(const std::string &value) {
  uint8_t mode = 0;
  bool found = false;
  for (size_t i = 0; i < LIGHT_EFFECT_MODES.size(); i++) {
    if (value == LIGHT_EFFECT_MODES[i]) {
      mode = static_cast<uint8_t>(i);
      found = true;
      break;
    }
  }

  if (!found) {
    ESP_LOGW(TAG, "Unsupported light effect option '%s'", value.c_str());
    return;
  }

  if (!this->request_light_effect_mode_(mode)) {
    ESP_LOGW(TAG, "Light effect request rejected");
    return;
  }
}

void SpaNetLightEffectSelect::handle_state_update_(const State &state) {
  if (state.light.effect_mode >= LIGHT_EFFECT_MODES.size()) {
    return;
  }

  if (this->has_published_state_ && this->last_effect_mode_ == state.light.effect_mode) {
    return;
  }

  this->publish_state(LIGHT_EFFECT_MODES[state.light.effect_mode]);
  this->has_published_state_ = true;
  this->last_effect_mode_ = state.light.effect_mode;
}

bool SpaNetLightEffectSelect::request_light_effect_mode_(uint8_t mode) {
  if (mode > 4) {
    ESP_LOGW(TAG, "Rejected invalid light effect mode %u (max 4)", mode);
    return false;
  }
  const std::string mode_str = std::to_string(mode);
  this->parent_->enqueue_command_(QueuedCommand{
      .kind = CommandKind::kLightEffectMode,
      .payload = "S07:" + mode_str,
      .expected_ack = mode_str,
      .timeout_ms = COMMAND_TIMEOUT_MS,
  });
  return true;
}

}  // namespace esphome::spanet
