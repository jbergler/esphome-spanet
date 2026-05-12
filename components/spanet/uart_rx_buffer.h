#pragma once

#include <optional>
#include <string>

namespace esphome::spanet {

class UartRxBuffer {
public:
  explicit UartRxBuffer(size_t max_len = 256) : max_len_(max_len) {}

  std::optional<std::string> feed(uint8_t byte) {
    if (byte == '\n') {
      if (buffer_.empty()) {
        return std::nullopt;
      }
      auto out = buffer_;
      buffer_.clear();
      return out;
    }

    if (byte == '\r') {
      return std::nullopt;
    }

    if (buffer_.size() >= max_len_) {
      buffer_.clear();
      return std::nullopt;
    }

    buffer_.push_back(static_cast<char>(byte));
    return std::nullopt;
  }

  void clear() { buffer_.clear(); }

  bool empty() const { return buffer_.empty(); }

private:
  std::string buffer_;
  size_t max_len_;
};

} // namespace esphome::spanet
