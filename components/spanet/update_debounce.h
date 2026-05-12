#pragma once

#include <chrono>
#include <cstdint>
#include <limits>

namespace esphome::spanet {

class UpdateDebounceGate {
public:
  using ClockFn = uint32_t (*)();

  explicit UpdateDebounceGate(
      uint32_t timeout_ms,
      ClockFn clock_fn = &UpdateDebounceGate::default_now_ms_)
      : timeout_ms_(timeout_ms), clock_fn_(clock_fn) {}

  bool try_arm() {
    const uint32_t now_ms = this->clock_fn_();

    if (!this->is_armed()) {
      this->armed_at_ms_ = now_ms;
      return true;
    }

    if (this->timeout_ms_ > 0 &&
        static_cast<uint32_t>(now_ms - this->armed_at_ms_) >=
            this->timeout_ms_) {
      this->armed_at_ms_ = now_ms;
      return true;
    }

    return false;
  }

  void disarm() { this->armed_at_ms_ = kDisarmedMs; }

  bool is_armed() const { return this->armed_at_ms_ != kDisarmedMs; }

private:
  static constexpr uint32_t kDisarmedMs = std::numeric_limits<uint32_t>::max();

  static uint32_t default_now_ms_() {
    const auto now = std::chrono::steady_clock::now();
    const auto since_epoch =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch())
            .count();
    return static_cast<uint32_t>(since_epoch);
  }

  uint32_t timeout_ms_{0};
  ClockFn clock_fn_;
  uint32_t armed_at_ms_{kDisarmedMs};
};

} // namespace esphome::spanet
