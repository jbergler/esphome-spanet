#pragma once

#if __has_include("esphome/core/log.h")
#include "esphome/core/log.h"
#else
#define ESP_LOGD(tag, format, ...) ((void) 0)
#define ESP_LOGV(tag, format, ...) ((void) 0)
#endif

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>

static const char *const TAG = "spanet.cmdq";

namespace esphome::spanet {

enum class CommandKind {
  kRfPoll,
  kSetpointWrite,
  kPumpWrite,
  kLightToggle,
  kLightBrightness,
  kLightColor,
  kLightEffectMode,
  kLightEffectSpeed,
};

struct Command {
  CommandKind kind;
  std::string payload;
  std::optional<std::string> expected_ack;
  // Optional second-stage completion check for multi-line responses.
  std::function<bool(const std::string &)> completion_predicate;
  uint32_t timeout_ms;
  bool triggers_rf_poll = false;
  // Optional callback to mutate state on successful ack
  std::function<void(class State &)> on_success;
};

struct InFlightCommand {
  Command command;
  uint32_t sent_at_ms;
  bool initial_ack_matched{false};
};

enum class EnqueueResult {
  kEnqueued,
  kDroppedDuplicateRfPoll,
  kDroppedQueueFull,
};

enum class AckResult {
  kNoInFlightCommand,
  kUnmatchedAck,
  kInProgress,
  kMatched,
};

class CommandQueue {
 public:
  using SendFn = std::function<void(const std::string &)>;
  using NowMsFn = std::function<uint32_t()>;

  CommandQueue(SendFn send_fn, NowMsFn now_ms_fn, size_t max_queued_commands);

  EnqueueResult enqueue(Command command);
  AckResult acknowledge(const std::string &message, InFlightCommand *matched_command);
  bool expire_timed_out(uint32_t now_ms, InFlightCommand *timed_out_command, uint32_t *age_ms);

  bool has_pending_kind(CommandKind kind) const;

 public:
  void maybe_send_next_();

  SendFn send_fn_;
  NowMsFn now_ms_fn_;
  size_t max_queued_commands_;
  std::unique_ptr<Command[]> queue_storage_;
  size_t queue_head_{0};
  size_t queue_size_{0};
  std::optional<InFlightCommand> in_flight_command_;
};

inline CommandQueue::CommandQueue(SendFn send_fn, NowMsFn now_ms_fn, size_t max_queued_commands)
    : send_fn_(std::move(send_fn)), now_ms_fn_(std::move(now_ms_fn)), max_queued_commands_(max_queued_commands) {
  if (this->max_queued_commands_ > 0) {
    this->queue_storage_ = std::make_unique<Command[]>(this->max_queued_commands_);
  }
}

inline EnqueueResult CommandQueue::enqueue(Command command) {
  ESP_LOGD(TAG, "enqueue called: kind=%d payload='%s'", static_cast<int>(command.kind), command.payload.c_str());
  if (command.kind == CommandKind::kRfPoll && this->has_pending_kind(CommandKind::kRfPoll)) {
    return EnqueueResult::kDroppedDuplicateRfPoll;
  }

  if (this->queue_size_ >= this->max_queued_commands_) {
    return EnqueueResult::kDroppedQueueFull;
  }

  const size_t tail = (this->queue_head_ + this->queue_size_) % this->max_queued_commands_;
  this->queue_storage_[tail] = std::move(command);
  this->queue_size_++;
  this->maybe_send_next_();
  return EnqueueResult::kEnqueued;
}

inline AckResult CommandQueue::acknowledge(const std::string &message, InFlightCommand *matched_command) {
  if (!this->in_flight_command_.has_value()) {
    return AckResult::kNoInFlightCommand;
  }

  auto &in_flight = this->in_flight_command_.value();
  if (in_flight.command.completion_predicate) {
    if (!in_flight.initial_ack_matched) {
      if (message != in_flight.command.expected_ack.value()) {
        ESP_LOGD(TAG, "Message '%s' did not match expected ack '%s' for in-flight command", message.c_str(),
                 in_flight.command.expected_ack.value().c_str());
        return AckResult::kUnmatchedAck;
      }

      in_flight.initial_ack_matched = true;
      return AckResult::kInProgress;
    }

    if (!in_flight.command.completion_predicate(message)) {
      return AckResult::kInProgress;
    }
  } else if (message != in_flight.command.expected_ack.value()) {
    ESP_LOGD(TAG, "Message '%s' did not match expected ack '%s' for in-flight command", message.c_str(),
             in_flight.command.expected_ack.value().c_str());
    return AckResult::kUnmatchedAck;
  }

  if (matched_command != nullptr) {
    *matched_command = in_flight;
  }

  this->in_flight_command_.reset();
  return AckResult::kMatched;
}

inline bool CommandQueue::expire_timed_out(uint32_t now_ms, InFlightCommand *timed_out_command, uint32_t *age_ms) {
  if (!this->in_flight_command_.has_value()) {
    return false;
  }

  if (this->in_flight_command_->command.timeout_ms == 0) {
    return false;
  }

  const uint32_t in_flight_age_ms = now_ms - this->in_flight_command_->sent_at_ms;
  if (in_flight_age_ms < this->in_flight_command_->command.timeout_ms) {
    return false;
  }

  if (timed_out_command != nullptr) {
    *timed_out_command = this->in_flight_command_.value();
  }
  if (age_ms != nullptr) {
    *age_ms = in_flight_age_ms;
  }

  this->in_flight_command_.reset();
  this->maybe_send_next_();
  return true;
}

inline bool CommandQueue::has_pending_kind(CommandKind kind) const {
  if (this->in_flight_command_.has_value() && this->in_flight_command_->command.kind == kind) {
    return true;
  }

  for (size_t i = 0; i < this->queue_size_; i++) {
    const size_t index = (this->queue_head_ + i) % this->max_queued_commands_;
    const auto &command = this->queue_storage_[index];
    if (command.kind == kind) {
      return true;
    }
  }

  return false;
}

inline void CommandQueue::maybe_send_next_() {
  if (this->in_flight_command_.has_value()) {
    return;
  }

  while (this->queue_size_ > 0) {
    auto next = std::move(this->queue_storage_[this->queue_head_]);
    this->queue_head_ = (this->queue_head_ + 1) % this->max_queued_commands_;
    this->queue_size_--;

    this->send_fn_(next.payload + "\n");

    if (!next.expected_ack.has_value()) {
      continue;
    }

    this->in_flight_command_ = InFlightCommand{
        .command = std::move(next),
        .sent_at_ms = this->now_ms_fn_(),
    };
    return;
  }
}

}  // namespace esphome::spanet
