#include "spanet.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "esphome/core/time.h"

#include <cstdio>

#if __has_include("esphome/components/time/real_time_clock.h")
#include "esphome/components/time/real_time_clock.h"
#define SPANET_HAS_REALTIME_CLOCK 1
#else
#define SPANET_HAS_REALTIME_CLOCK 0
#endif

namespace esphome::spanet {

static const char *const TAG = "spanet";
static constexpr const char *const STATE_UPDATE_DEBOUNCE_TIMEOUT = "state_update_debounce";
static constexpr const char *const INITIAL_POLL_TIMEOUT = "initial_poll";
static constexpr uint32_t STATE_UPDATE_DEBOUNCE_MS = 250;
static constexpr size_t MAX_QUEUED_COMMANDS = 4;
static constexpr uint32_t RF_POLL_TIMEOUT_MS = 5000;
static constexpr uint32_t INITIAL_POLL_DELAY_MS = 10000;
static constexpr uint32_t SET_TIME_COMMAND_TIMEOUT_MS = 1500;
static constexpr uint8_t SET_TIME_STEP_COUNT = 6;

void SpaNetComponent::setup() {
  ESP_LOGI(TAG, "Configuring SpaNET component");
  this->check_uart_settings(38400);

  this->command_queue_ =
      std::make_unique<CommandQueue>([this](const std::string &command) { this->send_uart_command_(command); },
                                     []() { return millis(); }, MAX_QUEUED_COMMANDS);

  this->add_on_state_callback([this](const State &state) {
    const auto &controller = state.controller_status;
    const auto &temperatures = state.temperatures;
    const auto &power = state.power;

    // Controller information
    this->publish_text_sensor_if_changed_(this->sen_controller_model_, controller.model);
    this->publish_text_sensor_if_changed_(this->sen_controller_fw_version_, controller.software_version);
    this->publish_text_sensor_if_changed_(this->sen_controller_serial_, controller.serial_number);
    if (controller.current_time.has_value()) {
      this->publish_text_sensor_if_changed_(this->sen_current_time_,
                                            format_time_text_(controller.current_time.value()));
    }

    // Temperatures
    this->publish_float_sensor_if_changed_(this->sen_water_temperature_, temperatures.water_c);
    this->publish_float_sensor_if_changed_(this->sen_setpoint_temperature_, temperatures.setpoint_c);
    this->publish_float_sensor_if_changed_(this->sen_heater_temperature_, temperatures.heater_c);
    this->publish_float_sensor_if_changed_(this->sen_case_temperature_, temperatures.case_c);

    // Power
    this->publish_float_sensor_if_changed_(this->sen_mains_voltage_, power.mains_voltage_v);
    this->publish_float_sensor_if_changed_(this->sen_mains_current_, power.mains_current_a);
    this->publish_float_sensor_if_changed_(this->sen_instant_power_, power.instant_power_w);
    this->publish_float_sensor_if_changed_(this->sen_total_energy_, power.total_energy_kwh);
  });

  // Trigger initial poll after startup delay; PollingComponent handles
  // recurring polls.
  this->set_timeout(INITIAL_POLL_TIMEOUT, INITIAL_POLL_DELAY_MS, [this]() { this->update(); });
}

void SpaNetComponent::loop() {
  while (this->available() > 0) {
    uint8_t byte;
    if (!this->read_byte(&byte)) {
      break;
    }

    auto maybe_message = this->rx_buffer_.feed(byte);
    if (maybe_message.has_value()) {
      this->on_uart_message_(maybe_message.value());
    }
  }

  const uint32_t now_ms = millis();
  this->process_command_timeouts_(now_ms);
  if (this->command_queue_ != nullptr) {
    this->command_queue_->maybe_send_next_();
  }

  this->maybe_run_auto_time_sync_(now_ms);
}

void SpaNetComponent::on_uart_message_(const std::string &message) {
  ESP_LOGI(TAG, "UART RX: %s", message.c_str());

  if (this->command_queue_ == nullptr) {
    return;
  }

  InFlightCommand matched_command;
  switch (this->command_queue_->acknowledge(message, &matched_command)) {
    case AckResult::kNoInFlightCommand:
      break;
    case AckResult::kUnmatchedAck:
      ESP_LOGV(TAG, "Message '%s' did not match expected ack for in-flight command", message.c_str());
      break;
    case AckResult::kInProgress:
      // Intermediate RF poll lines must still update the register store (e.g. R3
      // sets major_version; it only ever arrives inside an RF poll response).
      break;
    case AckResult::kMatched:
      // Pre-emptively mutate state if on_success callback is set
      if (matched_command.command.on_success) {
        matched_command.command.on_success(this->register_store_.get_mutable_state());
        this->notify_state_update_(this->register_store_.get_state());
      }
      if (matched_command.command.triggers_rf_poll) {
        this->update();
      }
      // RF poll completion triggers callbacks immediately (no need for debounce flag)
      if (matched_command.command.kind == CommandKind::kRfPoll) {
        for (auto &callback : this->rf_poll_complete_callbacks_) {
          callback();
        }
      }
      // If the matched message is also a state update (e.g., RG/RE sentinel for staged RF),
      // fall through to process it as such.
      if (SpaNetParser::classify_message(message) == MessageType::kStateUpdate) {
        break;
      }
      return;
  }

  switch (SpaNetParser::classify_message(message)) {
    case MessageType::kStateUpdate:
      this->on_state_update_message_(message);
      break;
    case MessageType::kAck:
      break;
    case MessageType::kUnknown:
      ESP_LOGW(TAG, "Ignoring unknown message '%s'", message.c_str());
      break;
  }
}

void SpaNetComponent::on_state_update_message_(const std::string &message) {
  if (!this->register_store_.update(message)) {
    ESP_LOGW(TAG, "Failed to update register store for SpaNET state payload");
    return;
  }

  if (!this->state_update_debounce_.try_arm()) {
    return;
  }

  this->set_timeout(STATE_UPDATE_DEBOUNCE_TIMEOUT, STATE_UPDATE_DEBOUNCE_MS, [this]() {
    this->state_update_debounce_.disarm();
    this->notify_state_update_(this->register_store_.get_state());
  });
}

void SpaNetComponent::update() {
  ESP_LOGV(TAG, "Polling for state");
  this->enqueue_command_(Command{
      .kind = CommandKind::kRfPoll,
      .payload = "RF",
      .expected_acks = {"RF:"},
      .completion_predicate = make_rf_completion_predicate(this->register_store_),
      .timeout_ms = RF_POLL_TIMEOUT_MS,
      .triggers_rf_poll = false,
  });
}

void SpaNetComponent::notify_state_update_(const State &state) {
  for (auto &callback : this->state_callbacks_) {
    callback(state);
  }
}

void SpaNetComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "SpaNET dummy component");
  LOG_UPDATE_INTERVAL(this);
  if (this->sen_controller_model_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Model: %s", this->sen_controller_model_->get_name().c_str());
  }
  if (this->sen_controller_serial_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Serial: %s", this->sen_controller_serial_->get_name().c_str());
  }
  if (this->sen_controller_fw_version_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Firmware Version: %s", this->sen_controller_fw_version_->get_name().c_str());
  }
  if (this->sen_current_time_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Current Time: %s", this->sen_current_time_->get_name().c_str());
  }
  if (this->sen_water_temperature_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Water Temperature: %s", this->sen_water_temperature_->get_name().c_str());
  }
  if (this->sen_setpoint_temperature_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Setpoint Temperature: %s", this->sen_setpoint_temperature_->get_name().c_str());
  }
  if (this->sen_heater_temperature_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Heater Temperature: %s", this->sen_heater_temperature_->get_name().c_str());
  }
  if (this->sen_case_temperature_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Case Temperature: %s", this->sen_case_temperature_->get_name().c_str());
  }
  if (this->sen_mains_voltage_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Mains Voltage: %s", this->sen_mains_voltage_->get_name().c_str());
  }
  if (this->sen_mains_current_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Mains Current: %s", this->sen_mains_current_->get_name().c_str());
  }
  if (this->sen_instant_power_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Instant Power: %s", this->sen_instant_power_->get_name().c_str());
  }
  if (this->sen_total_energy_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Total Energy: %s", this->sen_total_energy_->get_name().c_str());
  }
  if (this->time_source_ != nullptr) {
    ESP_LOGCONFIG(TAG, "  Auto Sync Time: %s", YESNO(this->auto_sync_time_));
    ESP_LOGCONFIG(TAG, "  Auto Sync Interval (ms): %u", static_cast<unsigned>(this->auto_sync_interval_ms_));
  }
}

void SpaNetComponent::enqueue_command_(Command command) {
  if (this->command_queue_ == nullptr) {
    return;
  }

  const CommandKind kind = command.kind;
  const std::string payload = command.payload;
  switch (this->command_queue_->enqueue(std::move(command))) {
    case EnqueueResult::kEnqueued:
      return;
    case EnqueueResult::kDroppedDuplicateRfPoll:
      ESP_LOGV(TAG, "Dropping duplicate RF poll while one is queued or in-flight");
      return;
    case EnqueueResult::kDroppedQueueFull:
      ESP_LOGW(TAG, "Dropping command due to full queue (max=%u): cmd=%s", static_cast<unsigned>(MAX_QUEUED_COMMANDS),
               payload.c_str());
      return;
  }
}

bool SpaNetComponent::has_pending_command_kind_(CommandKind kind) const {
  if (this->command_queue_ == nullptr) {
    return false;
  }
  return this->command_queue_->has_pending_kind(kind);
}

void SpaNetComponent::process_command_timeouts_(uint32_t now_ms) {
  if (this->command_queue_ == nullptr) {
    return;
  }

  InFlightCommand timed_out_command;
  uint32_t age_ms = 0;
  if (!this->command_queue_->expire_timed_out(now_ms, &timed_out_command, &age_ms)) {
    return;
  }

  ESP_LOGW(TAG, "Command timed out after %u ms: payload=%s", age_ms, timed_out_command.command.payload.c_str());

  if (timed_out_command.command.kind != CommandKind::kSetTimeWrite || !this->time_sync_in_progress_) {
    return;
  }

  const auto maybe_step_index = parse_time_sync_step_index_(timed_out_command.command.payload);
  if (!maybe_step_index.has_value() || maybe_step_index.value() != this->time_sync_step_index_) {
    this->abort_time_sync_();
    return;
  }

  if (this->time_sync_retry_used_) {
    ESP_LOGW(TAG, "Aborting set-time sequence after retry exhausted at step S%02u",
             static_cast<unsigned>(this->time_sync_step_index_ + 1));
    this->abort_time_sync_();
    return;
  }

  ESP_LOGW(TAG, "Retrying set-time step S%02u after timeout", static_cast<unsigned>(this->time_sync_step_index_ + 1));
  this->enqueue_time_sync_step_(this->time_sync_step_index_, true);
}

void SpaNetComponent::send_uart_command_(const std::string &command) {
  ESP_LOGI(TAG, "UART TX: %s", command.c_str());
  this->write_str(command.c_str());
}

bool SpaNetComponent::request_set_current_time(time_t unix_time) {
  if (unix_time <= 0) {
    ESP_LOGW(TAG, "Ignoring invalid unix time %ld", static_cast<long>(unix_time));
    return false;
  }

  if (this->time_sync_in_progress_ || this->has_pending_command_kind_(CommandKind::kSetTimeWrite)) {
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
  this->time_sync_retry_used_ = false;

  if (!this->enqueue_time_sync_step_(0, false)) {
    this->abort_time_sync_();
    return false;
  }

  return true;
}

bool SpaNetComponent::request_set_current_time_now() {
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

std::string SpaNetComponent::format_time_text_(time_t unix_time) {
  auto local_time = ESPTime::from_epoch_local(unix_time);
  if (!local_time.is_valid()) {
    return "";
  }

  return local_time.strftime("%Y-%m-%d %H:%M");
}

std::optional<uint8_t> SpaNetComponent::parse_time_sync_step_index_(const std::string &payload) {
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

std::optional<Command> SpaNetComponent::make_time_sync_command_(const std::tm &tm_value, uint8_t step_index,
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
  };
}

void SpaNetComponent::maybe_run_auto_time_sync_(uint32_t now_ms) {
  if (!this->auto_sync_time_ || this->time_source_ == nullptr || this->auto_sync_interval_ms_ == 0) {
    return;
  }

  if (now_ms < this->next_auto_sync_ms_) {
    return;
  }

  if (this->time_sync_in_progress_ || this->has_pending_command_kind_(CommandKind::kSetTimeWrite)) {
    this->next_auto_sync_ms_ = now_ms + 1000;
    return;
  }

  if (this->request_set_current_time_now()) {
    this->next_auto_sync_ms_ = now_ms + this->auto_sync_interval_ms_;
  } else {
    this->next_auto_sync_ms_ = now_ms + 5000;
  }
}

bool SpaNetComponent::enqueue_time_sync_step_(uint8_t step_index, bool retry) {
  const auto maybe_command = make_time_sync_command_(
      this->time_sync_tm_, step_index,
      [this, step_index](State &) { this->handle_time_sync_step_success_(step_index); }, SET_TIME_COMMAND_TIMEOUT_MS);
  if (!maybe_command.has_value()) {
    return false;
  }

  this->time_sync_step_index_ = step_index;
  this->time_sync_retry_used_ = retry;
  this->enqueue_command_(maybe_command.value());
  if (!this->has_pending_command_kind_(CommandKind::kSetTimeWrite)) {
    ESP_LOGW(TAG, "Failed to enqueue set-time step S%02u", static_cast<unsigned>(step_index + 1));
    return false;
  }

  return true;
}

void SpaNetComponent::handle_time_sync_step_success_(uint8_t step_index) {
  if (!this->time_sync_in_progress_ || this->time_sync_step_index_ != step_index) {
    return;
  }

  if (step_index + 1 >= SET_TIME_STEP_COUNT) {
    this->time_sync_in_progress_ = false;
    this->time_sync_step_index_ = 0;
    this->time_sync_retry_used_ = false;
    this->update();
    return;
  }

  if (!this->enqueue_time_sync_step_(step_index + 1, false)) {
    this->abort_time_sync_();
  }
}

void SpaNetComponent::abort_time_sync_() {
  this->time_sync_in_progress_ = false;
  this->time_sync_step_index_ = 0;
  this->time_sync_retry_used_ = false;
}

}  // namespace esphome::spanet
