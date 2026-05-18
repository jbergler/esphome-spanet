#include "spanet_operating_mode_select.h"

#include "esphome/core/log.h"

namespace esphome::spanet {

static const char *const TAG = "spanet.operating_mode";
static constexpr uint32_t COMMAND_TIMEOUT_MS = 1000;

// Human-readable option labels, index == W66 command value.
static constexpr std::array<const char *, 4> OPERATING_MODE_OPTIONS{{"Normal", "Economy", "Away", "Weekdays"}};

// Device register strings returned by R4.mode, indexed to match OPERATING_MODE_OPTIONS.
static constexpr std::array<const char *, 4> OPERATING_MODE_REGISTER_STRINGS{{"NORM", "ECON", "AWAY", "WEEK"}};

void SpaNetOperatingModeSelect::setup() {
  this->parent_->add_on_state_callback([this](const State &state) { this->handle_state_update_(state); });
  this->handle_state_update_(this->parent_->get_state());
}

void SpaNetOperatingModeSelect::dump_config() { LOG_SELECT("", "SpaNET Operating Mode", this); }

void SpaNetOperatingModeSelect::control(const std::string &value) {
  uint8_t mode = 0;
  bool found = false;
  for (size_t i = 0; i < OPERATING_MODE_OPTIONS.size(); i++) {
    if (value == OPERATING_MODE_OPTIONS[i]) {
      mode = static_cast<uint8_t>(i);
      found = true;
      break;
    }
  }

  if (!found) {
    ESP_LOGW(TAG, "Unsupported operating mode option '%s'", value.c_str());
    return;
  }

  if (!this->request_operating_mode_(mode)) {
    ESP_LOGW(TAG, "Operating mode request rejected");
    return;
  }
}

void SpaNetOperatingModeSelect::handle_state_update_(const State &state) {
  if (!state.spa_operating.operating_mode.has_value()) {
    return;
  }

  const std::string &reg_mode = state.spa_operating.operating_mode.value();

  if (this->has_published_state_ && this->last_mode_ == reg_mode) {
    return;
  }

  for (size_t i = 0; i < OPERATING_MODE_REGISTER_STRINGS.size(); i++) {
    if (reg_mode == OPERATING_MODE_REGISTER_STRINGS[i]) {
      this->publish_state(OPERATING_MODE_OPTIONS[i]);
      this->has_published_state_ = true;
      this->last_mode_ = reg_mode;
      return;
    }
  }

  ESP_LOGW(TAG, "Unknown operating mode register value '%s'", reg_mode.c_str());
}

bool SpaNetOperatingModeSelect::request_operating_mode_(uint8_t mode) {
  if (mode >= OPERATING_MODE_OPTIONS.size()) {
    ESP_LOGW(TAG, "Rejected invalid operating mode %u (max %zu)", mode, OPERATING_MODE_OPTIONS.size() - 1);
    return false;
  }
  const std::string mode_str = std::to_string(mode);
  this->parent_->enqueue_command_(Command{
      .kind = CommandKind::kOperatingModeWrite,
      .payload = "W66:" + mode_str,
      .expected_acks = {mode_str},
      .timeout_ms = COMMAND_TIMEOUT_MS,
      .triggers_rf_poll = true,
  });
  return true;
}

}  // namespace esphome::spanet
