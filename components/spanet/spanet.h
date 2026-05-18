#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "esphome/components/uart/uart.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"

#include "command_queue.h"
#include "spanet_parser.h"
#include "spanet_state.h"
#include "spanet_time_sync.h"
#include "uart_rx_buffer.h"
#include "update_debounce.h"

namespace esphome::spanet {

class SpaNetComponent : public PollingComponent, public uart::UARTDevice {
 public:
  using StateUpdateCallback = std::function<void(const State &)>;

  void set_time_source(esphome::time::RealTimeClock *time_source) { this->time_source_ = time_source; }
  void set_auto_sync_time(bool auto_sync_time) { this->auto_sync_time_ = auto_sync_time; }
  void set_auto_sync_interval_ms(uint32_t interval_ms) { this->auto_sync_interval_ms_ = interval_ms; }
  void add_on_state_callback(StateUpdateCallback callback) { this->state_callbacks_.push_back(std::move(callback)); }
  const State &get_state() const { return this->register_store_.get_state(); }
  bool is_spa_data_fresh(uint32_t timeout_ms = 120000) const;
  bool request_set_current_time(time_t unix_time);
  bool request_set_current_time_now();
  void enqueue_command_(Command command);
  bool has_pending_command_kind_(CommandKind kind) const;

  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;

 protected:
  void on_uart_message_(const std::string &message);
  void on_state_update_message_(const std::string &message);
  void notify_state_update_(const State &state);
  void process_command_timeouts_(uint32_t now_ms);
  virtual void send_uart_command_(const std::string &command);

  UartRxBuffer rx_buffer_{256};
  RegisterStore register_store_;
  UpdateDebounceGate state_update_debounce_{250};
  std::vector<StateUpdateCallback> state_callbacks_;
  std::unique_ptr<CommandQueue> command_queue_;
  std::unique_ptr<SpaNetTimeSync> time_sync_;

  uint32_t last_rf_poll_ms_{0};

  // Staging values for time sync configuration (set before setup() is called).
  esphome::time::RealTimeClock *time_source_{nullptr};
  bool auto_sync_time_{false};
  uint32_t auto_sync_interval_ms_{3600000};
};

template<typename... Ts> class SpaNetSetCurrentTimeAction : public Action<Ts...> {
 public:
  explicit SpaNetSetCurrentTimeAction(SpaNetComponent *parent) : parent_(parent) {}

  TEMPLATABLE_VALUE(uint32_t, unix_timestamp)

  void play(Ts... x) override {
    if (this->unix_timestamp_.has_value()) {
      this->parent_->request_set_current_time(static_cast<time_t>(this->unix_timestamp_.value(x...)));
      return;
    }
    this->parent_->request_set_current_time_now();
  }

 protected:
  SpaNetComponent *parent_;
};

}  // namespace esphome::spanet
