#include "spanet_time_sync.h"
#include "spanet.h"
#include "esphome/core/log.h"
#include "esphome/core/time.h"

#if __has_include("esphome/components/time/real_time_clock.h")
#include "esphome/components/time/real_time_clock.h"
#define SPANET_HAS_REALTIME_CLOCK 1
#else
#define SPANET_HAS_REALTIME_CLOCK 0
#endif

#include <cstdio>

namespace esphome::spanet {

static const char *const TAG = "spanet";

SpaNetTimeSync::SpaNetTimeSync(SpaNetComponent *parent) : parent_(parent) {}

void SpaNetTimeSync::loop(uint32_t now_ms) { this->maybe_run_auto_time_sync(now_ms); }

void SpaNetTimeSync::dump_config() const {
  if (this->time_source_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Auto Sync Time: %s", YESNO(this->auto_sync_time_));
    ESP_LOGCONFIG(TAG, "  Auto Sync Interval (ms): %u", static_cast<unsigned>(this->auto_sync_interval_ms_));
  }
}

void SpaNetTimeSync::set_time_source(esphome::time::RealTimeClock *time_source) { this->time_source_ = time_source; }

void SpaNetTimeSync::set_auto_sync(bool enabled, uint32_t interval_ms) {
  this->auto_sync_time_ = enabled;
  this->auto_sync_interval_ms_ = interval_ms;
}

bool SpaNetTimeSync::request_set_current_time(time_t unix_time) {
  if (unix_time <= 0) {
    ESP_LOGW(TAG, "Ignoring invalid unix time %ld", static_cast<long>(unix_time));
    return false;
  }

  if (this->time_sync_in_progress_ || this->parent_->has_pending_command_kind_(CommandKind::kSetTimeWrite)) {
    ESP_LOGV(TAG, "Ignoring set-time request while one is already in progress");
    return false;
  }

  const auto local_time = ESPTime::from_epoch_local(unix_time);
  if (!local_time.is_valid()) {
    ESP_LOGW(TAG, "Failed to convert unix time for set-time request");
    return false;
  }

  this->time_sync_tm_.tm_year = static_cast<int>(local_time.year) - 1900;
  this->time_sync_tm_.tm_mon = static_cast<int>(local_time.month) - 1;
  this->time_sync_tm_.tm_mday = static_cast<int>(local_time.day_of_month);
  this->time_sync_tm_.tm_hour = static_cast<int>(local_time.hour);
  this->time_sync_tm_.tm_min = static_cast<int>(local_time.minute);
  this->time_sync_tm_.tm_sec = static_cast<int>(local_time.second);
  this->time_sync_tm_.tm_wday = static_cast<int>(local_time.day_of_week % 7);
  this->time_sync_tm_.tm_yday = static_cast<int>(local_time.day_of_year) - 1;
  this->time_sync_in_progress_ = true;
  this->time_sync_step_index_ = 0;
  this->time_sync_retry_count_ = 0;

  if (!this->enqueue_time_sync_step(0)) {
    this->abort_time_sync();
    return false;
  }

  return true;
}

bool SpaNetTimeSync::request_set_current_time_now() {
  if (this->time_source_ == nullptr) {
    ESP_LOGW(TAG, "Cannot set current time without configured time source");
    return false;
  }

#if SPANET_HAS_REALTIME_CLOCK
  auto now = this->time_source_->now();
  if (!now.is_valid()) {
    ESP_LOGW(TAG, "Time source is not valid yet; skipping set-time request");
    return false;
  }

  return this->request_set_current_time(static_cast<time_t>(now.timestamp));
#else
  ESP_LOGW(TAG, "Set-time-now unavailable: RealTimeClock header not present in this build context");
  return false;
#endif
}

std::string SpaNetTimeSync::format_time_text(time_t unix_time) {
  auto local_time = ESPTime::from_epoch_local(unix_time);
  if (!local_time.is_valid()) {
    return "";
  }

  return local_time.strftime("%Y-%m-%d %H:%M");
}

void SpaNetTimeSync::on_command_timeout(const InFlightCommand &timed_out, uint32_t age_ms) {
  if (timed_out.command.kind != CommandKind::kSetTimeWrite || !this->time_sync_in_progress_) {
    return;
  }

  const auto maybe_step_index = parse_time_sync_step_index(timed_out.command.payload);
  if (!maybe_step_index.has_value() || maybe_step_index.value() != this->time_sync_step_index_) {
    this->abort_time_sync();
    return;
  }

  this->time_sync_retry_count_++;
  if (this->time_sync_retry_count_ > SET_TIME_MAX_RETRIES) {
    ESP_LOGW(TAG, "Aborting set-time sequence after %u attempts at step S%02u",
             static_cast<unsigned>(SET_TIME_MAX_RETRIES + 1), static_cast<unsigned>(this->time_sync_step_index_ + 1));
    this->abort_time_sync();
    return;
  }

  ESP_LOGW(TAG, "Retrying set-time step S%02u (attempt %u of %u)",
           static_cast<unsigned>(this->time_sync_step_index_ + 1),
           static_cast<unsigned>(this->time_sync_retry_count_ + 1), static_cast<unsigned>(SET_TIME_MAX_RETRIES + 1));
  this->enqueue_time_sync_step(this->time_sync_step_index_);
}

std::optional<uint8_t> SpaNetTimeSync::parse_time_sync_step_index(const std::string &payload) {
  if (payload.size() < 3 || payload[0] != 'S' || payload[1] < '0' || payload[1] > '9' || payload[2] < '0' ||
      payload[2] > '9') {
    return std::nullopt;
  }

  const int command_number = (payload[1] - '0') * 10 + (payload[2] - '0');
  if (command_number < 1 || command_number > SET_TIME_STEP_COUNT) {
    return std::nullopt;
  }

  return static_cast<uint8_t>(command_number - 1);
}

std::optional<Command> SpaNetTimeSync::make_time_sync_command(const std::tm &tm_value, uint8_t step_index,
                                                              std::function<void(State &)> on_success,
                                                              uint32_t timeout_ms) {
  if (step_index >= SET_TIME_STEP_COUNT) {
    return std::nullopt;
  }

  int value = 0;
  switch (step_index) {
    case 0:
      value = tm_value.tm_year + 1900;
      break;
    case 1:
      value = tm_value.tm_mon + 1;
      break;
    case 2:
      value = tm_value.tm_mday;
      break;
    case 3:
      value = tm_value.tm_hour;
      break;
    case 4:
      value = tm_value.tm_min;
      break;
    case 5:
      value = (tm_value.tm_wday + 6) % 7;
      break;
    default:
      return std::nullopt;
  }

  char command_prefix[4];
  std::snprintf(command_prefix, sizeof(command_prefix), "S%02u", static_cast<unsigned>(step_index + 1));
  const std::string value_str = std::to_string(value);
  const std::string command_str = std::string(command_prefix);
  return Command{
      .kind = CommandKind::kSetTimeWrite,
      .payload = command_str + ":" + value_str,
      .expected_acks = {value_str, command_str},
      .timeout_ms = timeout_ms,
      .triggers_rf_poll = false,
      .on_success = std::move(on_success),
      .allow_duplicates = true,
  };
}

bool SpaNetTimeSync::enqueue_time_sync_step(uint8_t step_index) {
  uint32_t timeout_ms = SET_TIME_COMMAND_TIMEOUT_MS;
  for (uint8_t i = 0; i < this->time_sync_retry_count_; i++) {
    timeout_ms *= 2;
    if (timeout_ms > SET_TIME_MAX_TIMEOUT_MS) {
      timeout_ms = SET_TIME_MAX_TIMEOUT_MS;
      break;
    }
  }
  const auto maybe_command = make_time_sync_command(
      this->time_sync_tm_, step_index, [this, step_index](State &) { this->handle_time_sync_step_success(step_index); },
      timeout_ms);
  if (!maybe_command.has_value()) {
    return false;
  }

  this->time_sync_step_index_ = step_index;
  this->parent_->enqueue_command_(maybe_command.value());
  if (!this->parent_->has_pending_command_kind_(CommandKind::kSetTimeWrite)) {
    ESP_LOGW(TAG, "Failed to enqueue set-time step S%02u", static_cast<unsigned>(step_index + 1));
    return false;
  }

  return true;
}

void SpaNetTimeSync::handle_time_sync_step_success(uint8_t step_index) {
  if (!this->time_sync_in_progress_ || this->time_sync_step_index_ != step_index) {
    return;
  }

  if (step_index + 1 >= SET_TIME_STEP_COUNT) {
    this->time_sync_in_progress_ = false;
    this->time_sync_step_index_ = 0;
    this->time_sync_retry_count_ = 0;
    this->parent_->update();
    return;
  }

  this->time_sync_retry_count_ = 0;
  if (!this->enqueue_time_sync_step(step_index + 1)) {
    this->abort_time_sync();
  }
}

void SpaNetTimeSync::abort_time_sync() {
  this->time_sync_in_progress_ = false;
  this->time_sync_step_index_ = 0;
  this->time_sync_retry_count_ = 0;
}

void SpaNetTimeSync::maybe_run_auto_time_sync(uint32_t now_ms) {
  if (!this->auto_sync_time_ || this->time_source_ == nullptr || this->auto_sync_interval_ms_ == 0) {
    return;
  }

  if (now_ms < this->next_auto_sync_ms_) {
    return;
  }

  if (this->time_sync_in_progress_ || this->parent_->has_pending_command_kind_(CommandKind::kSetTimeWrite)) {
    this->next_auto_sync_ms_ = now_ms + 1000;
    return;
  }

  if (this->request_set_current_time_now()) {
    this->next_auto_sync_ms_ = now_ms + this->auto_sync_interval_ms_;
  } else {
    this->next_auto_sync_ms_ = now_ms + 5000;
  }
}

}  // namespace esphome::spanet
