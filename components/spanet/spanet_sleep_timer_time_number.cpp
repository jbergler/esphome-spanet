#include "spanet_sleep_timer_time_number.h"

#include <cmath>

#include "esphome/core/log.h"

namespace esphome::spanet {

static const char *const TAG = "spanet.sleep_timer_time";
static constexpr uint32_t COMMAND_TIMEOUT_MS = 1000;

void SpaNetSleepTimerTimeNumber::setup() {
  this->parent_->add_on_state_callback([this](const State &state) { this->handle_state_update_(state); });
  this->handle_state_update_(this->parent_->get_state());
}

void SpaNetSleepTimerTimeNumber::dump_config() { LOG_NUMBER("", "SpaNET Sleep Timer Time", this); }

void SpaNetSleepTimerTimeNumber::control(float value) {
  if (!this->has_published_state_) {
    ESP_LOGW(TAG, "Sleep timer time not yet initialized from spa state; ignoring write");
    return;
  }

  const int int_val = static_cast<int>(std::lround(value));

  int new_wire_val;
  if (this->field_ == SleepTimerField::kHour) {
    if (int_val < 0 || int_val > 23) {
      ESP_LOGW(TAG, "Hour value %d out of range (0-23)", int_val);
      return;
    }
    const int current_minute = this->last_wire_val_ % 256;
    new_wire_val = int_val * 256 + current_minute;
  } else {
    if (int_val < 0 || int_val > 59) {
      ESP_LOGW(TAG, "Minute value %d out of range (0-59)", int_val);
      return;
    }
    const int current_hour = this->last_wire_val_ / 256;
    new_wire_val = current_hour * 256 + int_val;
  }

  if (!this->request_time_(new_wire_val)) {
    ESP_LOGW(TAG, "Sleep timer time request rejected");
  }
}

void SpaNetSleepTimerTimeNumber::handle_state_update_(const State &state) {
  const std::optional<int> *wire_field;
  if (this->timer_index_ == 1) {
    wire_field = (this->endpoint_ == SleepTimerEndpoint::kBegin) ? &state.sleep_timers.timer1_begin_wire
                                                                 : &state.sleep_timers.timer1_end_wire;
  } else {
    wire_field = (this->endpoint_ == SleepTimerEndpoint::kBegin) ? &state.sleep_timers.timer2_begin_wire
                                                                 : &state.sleep_timers.timer2_end_wire;
  }

  if (!wire_field->has_value()) {
    return;
  }

  const int wire_val = wire_field->value();
  const int hh = wire_val / 256;
  const int mm = wire_val % 256;

  if (hh > 23 || mm > 59) {
    ESP_LOGW(TAG, "Invalid wire time value %d (hh=%d mm=%d)", wire_val, hh, mm);
    return;
  }

  const float published = (this->field_ == SleepTimerField::kHour) ? static_cast<float>(hh) : static_cast<float>(mm);

  if (this->has_published_state_ && this->last_wire_val_ == wire_val) {
    return;
  }

  this->last_wire_val_ = wire_val;
  this->publish_state(published);
  this->has_published_state_ = true;
}

bool SpaNetSleepTimerTimeNumber::request_time_(int new_wire_val) {
  CommandKind kind;
  std::string command_word;

  if (this->timer_index_ == 1) {
    if (this->endpoint_ == SleepTimerEndpoint::kBegin) {
      kind = CommandKind::kSleepTimer1BeginWrite;
      command_word = "W68";
    } else {
      kind = CommandKind::kSleepTimer1EndWrite;
      command_word = "W69";
    }
  } else {
    if (this->endpoint_ == SleepTimerEndpoint::kBegin) {
      kind = CommandKind::kSleepTimer2BeginWrite;
      command_word = "W71";
    } else {
      kind = CommandKind::kSleepTimer2EndWrite;
      command_word = "W72";
    }
  }

  const std::string wire_str = std::to_string(new_wire_val);
  this->parent_->enqueue_command_(Command{
      .kind = kind,
      .payload = command_word + ":" + wire_str,
      .expected_acks = {wire_str},
      .timeout_ms = COMMAND_TIMEOUT_MS,
      .triggers_rf_poll = true,
  });
  return true;
}

}  // namespace esphome::spanet
