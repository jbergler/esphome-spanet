#pragma once

#include <cstdint>
#include <cstdio>
#include <ctime>
#include <functional>
#include <optional>
#include <string>

#include "command_queue.h"
#include "spanet_state.h"

namespace esphome {
namespace time {
class RealTimeClock;
}
}  // namespace esphome

namespace esphome::spanet {

class SpaNetComponent;

class SpaNetTimeSync {
 public:
  explicit SpaNetTimeSync(SpaNetComponent *parent);

  void loop(uint32_t now_ms);
  void dump_config() const;

  bool request_set_current_time(time_t unix_time);
  bool request_set_current_time_now();
  static std::string format_time_text(time_t unix_time);

  void set_time_source(esphome::time::RealTimeClock *time_source);
  void set_auto_sync(bool enabled, uint32_t interval_ms);

  void on_command_timeout(const InFlightCommand &timed_out, uint32_t age_ms);

 private:
  SpaNetComponent *parent_;
  esphome::time::RealTimeClock *time_source_{nullptr};
  bool auto_sync_time_{false};
  uint32_t auto_sync_interval_ms_{0};
  uint32_t next_auto_sync_ms_{0};

  bool time_sync_in_progress_{false};
  uint8_t time_sync_step_index_{0};
  uint8_t time_sync_retry_count_{0};
  std::tm time_sync_tm_{};

  static constexpr uint32_t SET_TIME_COMMAND_TIMEOUT_MS = 1500;
  static constexpr uint32_t SET_TIME_MAX_TIMEOUT_MS = 10000;
  static constexpr uint8_t SET_TIME_MAX_RETRIES = 2;
  static constexpr time_t TIME_SYNC_SKIP_THRESHOLD_S = 60;
  static constexpr uint8_t SET_TIME_STEP_COUNT = 6;

  static std::optional<uint8_t> parse_time_sync_step_index(const std::string &payload);
  static std::optional<Command> make_time_sync_command(const std::tm &tm_value, uint8_t step_index,
                                                       std::function<void(State &)> on_success, uint32_t timeout_ms);
  bool enqueue_time_sync_step(uint8_t step_index);
  void handle_time_sync_step_success(uint8_t step_index);
  void abort_time_sync();
  void maybe_run_auto_time_sync(uint32_t now_ms);
};

}  // namespace esphome::spanet
