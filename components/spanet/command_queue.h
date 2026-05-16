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
#include <vector>
#include <utility>

static const char *const TAG = "spanet.cmdq";

namespace esphome::spanet {

enum class CommandKind {
  kRfPoll,
  kSetpointWrite,
  kPumpWrite,
  kSetTimeWrite,
  kLightToggle,
  kLightBrightness,
  kLightColor,
  kLightEffectMode,
  kLightEffectSpeed,
};

struct Command {
  CommandKind kind;
  std::string payload;
  std::vector<std::string> expected_acks;
  // Optional second-stage completion check for multi-line responses.
  std::function<bool(const std::string &)> completion_predicate;
  uint32_t timeout_ms;
  bool triggers_rf_poll = false;
  // Optional callback to mutate state on successful ack
  std::function<void(class State &)> on_success;
  bool allow_duplicates = false;
};

struct InFlightCommand {
  Command command;
  uint32_t sent_at_ms;
  size_t expected_ack_index{0};
  bool expected_ack_matched{false};
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
  kCompleted,
};

class CommandQueue {
 public:
  using SendFn = std::function<void(const std::string &)>;
  using NowMsFn = std::function<uint32_t()>;

  CommandQueue(SendFn send_fn, NowMsFn now_ms_fn, size_t max_queued_commands);

  EnqueueResult enqueue(Command command);
  AckResult acknowledge(const std::string &message, InFlightCommand *matched_command,
                        std::function<void(InFlightCommand &)> on_success = nullptr);
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
  ESP_LOGD(TAG, "enqueue called: kind=%d payload='%s', expected_acks=%zu", static_cast<int>(command.kind),
           command.payload.c_str(), command.expected_acks.size());
  if (!command.allow_duplicates && this->has_pending_kind(command.kind)) {
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

inline AckResult CommandQueue::acknowledge(const std::string &message, InFlightCommand *matched_command,
                                           std::function<void(InFlightCommand &)> on_success) {
  if (!this->in_flight_command_.has_value()) {
    return AckResult::kNoInFlightCommand;
  }

  auto &in_flight = this->in_flight_command_.value();
  bool found_match = false;

  if (in_flight.command.expected_acks.empty()) {
    in_flight.expected_ack_matched = true;
  }

  if (in_flight.expected_ack_index < in_flight.command.expected_acks.size()) {
    // Start by trying to match against the next expected ack
    if (message == in_flight.command.expected_acks[in_flight.expected_ack_index]) {
      if (matched_command != nullptr)
        *matched_command = in_flight;
      in_flight.expected_ack_index++;
      found_match = true;

      // Are we done?
      if (in_flight.expected_ack_index >= in_flight.command.expected_acks.size()) {
        in_flight.expected_ack_matched = true;
      }
    }
  }
  if (in_flight.expected_ack_matched) {
    bool completion_predicate_satisfied =
        !in_flight.command.completion_predicate || in_flight.command.completion_predicate(message);
    if (completion_predicate_satisfied) {
      if (matched_command != nullptr)
        *matched_command = in_flight;
      if (on_success) {
        on_success(in_flight);
      }
      this->in_flight_command_.reset();
      return AckResult::kCompleted;
    } else {
      return AckResult::kInProgress;
    }
  }

  if (found_match) {
    return AckResult::kInProgress;
  } else {
    auto remaining_ack_string = std::string();
    for (size_t i = in_flight.expected_ack_index; i < in_flight.command.expected_acks.size(); i++) {
      if (!remaining_ack_string.empty()) {
        remaining_ack_string += ", ";
      }
      remaining_ack_string += "'" + in_flight.command.expected_acks[i] + "'";
    }
    ESP_LOGD(TAG, "Message '%s' did not match any expected ack (%s) for in-flight command", message.c_str(),
             remaining_ack_string.c_str());
    return AckResult::kUnmatchedAck;
  }
}

inline bool CommandQueue::expire_timed_out(uint32_t now_ms, InFlightCommand *timed_out_command, uint32_t *age_ms) {
  if (!this->in_flight_command_.has_value()) {
    return false;
  }

  if (this->in_flight_command_->command.timeout_ms == 0) {
    return false;
  }

  if (now_ms < this->in_flight_command_->sent_at_ms) {
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

    if (next.expected_acks.empty()) {
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
