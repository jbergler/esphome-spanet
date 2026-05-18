#include "spanet_sleep_timer_day_select.h"

#include "esphome/core/log.h"

namespace esphome::spanet {

static const char *const TAG = "spanet.sleep_timer_day";
static constexpr uint32_t COMMAND_TIMEOUT_MS = 1000;

// Human-readable option labels; index must match DAY_PATTERN_RAWS below.
static constexpr std::array<const char *, 4> DAY_PATTERN_OPTIONS{{"Off", "Daily", "Weekends", "Weekdays"}};

// Raw register values corresponding to each label.
static constexpr std::array<int, 4> DAY_PATTERN_RAWS{{128, 127, 96, 31}};

void SpaNetSleepTimerDaySelect::setup() {
  this->parent_->add_on_state_callback([this](const State &state) { this->handle_state_update_(state); });
  this->handle_state_update_(this->parent_->get_state());
}

void SpaNetSleepTimerDaySelect::dump_config() { LOG_SELECT("", "SpaNET Sleep Timer Day", this); }

void SpaNetSleepTimerDaySelect::control(const std::string &value) {
  for (size_t i = 0; i < DAY_PATTERN_OPTIONS.size(); i++) {
    if (value == DAY_PATTERN_OPTIONS[i]) {
      if (!this->request_day_pattern_(DAY_PATTERN_RAWS[i])) {
        ESP_LOGW(TAG, "Sleep timer %u day pattern request rejected", this->timer_index_);
      }
      return;
    }
  }
  ESP_LOGW(TAG, "Unsupported day pattern option '%s'", value.c_str());
}

void SpaNetSleepTimerDaySelect::handle_state_update_(const State &state) {
  const auto &pattern =
      (this->timer_index_ == 1) ? state.sleep_timers.timer1_day_pattern : state.sleep_timers.timer2_day_pattern;
  if (!pattern.has_value()) {
    return;
  }

  const int raw = pattern.value();
  if (this->has_published_state_ && this->last_raw_value_ == raw) {
    return;
  }

  for (size_t i = 0; i < DAY_PATTERN_RAWS.size(); i++) {
    if (raw == DAY_PATTERN_RAWS[i]) {
      this->publish_state(DAY_PATTERN_OPTIONS[i]);
      this->has_published_state_ = true;
      this->last_raw_value_ = raw;
      return;
    }
  }

  ESP_LOGW(TAG, "Unknown sleep timer %u day pattern value %d", this->timer_index_, raw);
}

bool SpaNetSleepTimerDaySelect::request_day_pattern_(int raw_value) {
  CommandKind kind;
  std::string command_word;
  if (this->timer_index_ == 1) {
    kind = CommandKind::kSleepTimer1DayWrite;
    command_word = "W67";
  } else {
    kind = CommandKind::kSleepTimer2DayWrite;
    command_word = "W70";
  }
  const std::string raw_str = std::to_string(raw_value);
  this->parent_->enqueue_command_(Command{
      .kind = kind,
      .payload = command_word + ":" + raw_str,
      .expected_acks = {raw_str},
      .timeout_ms = COMMAND_TIMEOUT_MS,
      .triggers_rf_poll = true,
  });
  return true;
}

}  // namespace esphome::spanet
