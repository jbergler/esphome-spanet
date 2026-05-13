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
};

struct QueuedCommand {
  CommandKind kind;
  std::string payload;
  std::optional<std::string> expected_ack;
  uint32_t timeout_ms;
};

struct InFlightCommand {
  CommandKind kind;
  std::string payload;
  std::string expected_ack;
  uint32_t timeout_ms;
  uint32_t sent_at_ms;
};

enum class EnqueueResult {
  kEnqueued,
  kDroppedDuplicateRfPoll,
  kDroppedQueueFull,
};

enum class AckResult {
  kNoInFlightCommand,
  kUnmatchedAck,
  kMatched,
};

class CommandQueue {
 public:
  using SendFn = std::function<void(const std::string &)>;
  using NowMsFn = std::function<uint32_t()>;

  CommandQueue(SendFn send_fn, NowMsFn now_ms_fn, size_t max_queued_commands);

  EnqueueResult enqueue(QueuedCommand command);
  AckResult acknowledge(const std::string &message, InFlightCommand *matched_command);
  bool expire_timed_out(uint32_t now_ms, InFlightCommand *timed_out_command, uint32_t *age_ms);

  bool has_pending_kind(CommandKind kind) const;

 private:
  void maybe_send_next_();

  SendFn send_fn_;
  NowMsFn now_ms_fn_;
  size_t max_queued_commands_;
  std::unique_ptr<QueuedCommand[]> queue_storage_;
  size_t queue_head_{0};
  size_t queue_size_{0};
  std::optional<InFlightCommand> in_flight_command_;
};

inline CommandQueue::CommandQueue(SendFn send_fn, NowMsFn now_ms_fn, size_t max_queued_commands)
    : send_fn_(std::move(send_fn)), now_ms_fn_(std::move(now_ms_fn)), max_queued_commands_(max_queued_commands) {
  if (this->max_queued_commands_ > 0) {
    this->queue_storage_ = std::make_unique<QueuedCommand[]>(this->max_queued_commands_);
  }
}

inline EnqueueResult CommandQueue::enqueue(QueuedCommand command) {
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

  if (message != this->in_flight_command_->expected_ack) {
    ESP_LOGD(TAG, "Message '%s' did not match expected ack '%s' for in-flight command", message.c_str(),
             this->in_flight_command_->expected_ack.c_str());
    return AckResult::kUnmatchedAck;
  }

  if (matched_command != nullptr) {
    *matched_command = this->in_flight_command_.value();
  }

  this->in_flight_command_.reset();
  this->maybe_send_next_();
  return AckResult::kMatched;
}

inline bool CommandQueue::expire_timed_out(uint32_t now_ms, InFlightCommand *timed_out_command, uint32_t *age_ms) {
  if (!this->in_flight_command_.has_value()) {
    return false;
  }

  if (this->in_flight_command_->timeout_ms == 0) {
    return false;
  }

  const uint32_t in_flight_age_ms = now_ms - this->in_flight_command_->sent_at_ms;
  if (in_flight_age_ms < this->in_flight_command_->timeout_ms) {
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
  if (this->in_flight_command_.has_value() && this->in_flight_command_->kind == kind) {
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
        .kind = next.kind,
        .payload = std::move(next.payload),
        .expected_ack = next.expected_ack.value(),
        .timeout_ms = next.timeout_ms,
        .sent_at_ms = this->now_ms_fn_(),
    };
    return;
  }
}

}  // namespace esphome::spanet
