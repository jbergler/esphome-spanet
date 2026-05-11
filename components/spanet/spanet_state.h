#pragma once

#include <cerrno>
#include <cstdlib>
#include <cstdint>
#include <ctime>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "spanet_parser.h"

namespace esphome::spanet {

struct ControllerStatus {
  // Identity fields (from R3)
  std::string software_version;
  std::string model;
  std::string serial_number_1;
  std::string serial_number_2;

  // Timestamp in Unix epoch seconds (UTC), derived from R2 date/time fields.
  // Empty when R2 has not been received or date/time fields are invalid.
  std::optional<int64_t> controller_epoch;

  bool empty() const {
    return this->software_version.empty() && this->model.empty() && this->serial_number_1.empty() &&
           this->serial_number_2.empty();
  }

  // Builds a ControllerStatus from the raw R3 field list, optionally enriched
  // with R2 date/time converted to UTC epoch seconds. Returns nullopt if
  // r3_fields is too short to hold all required identity fields.
  //
  // R3 layout (0-based after the label): index 5 = software version,
  //   6 = model, 7 = serial 1, 8 = serial 2.
  // R2 layout: index 5 = hour, 6 = minute, 7 = second,
  //   8 = day, 9 = month, 10 = year.
  static std::optional<ControllerStatus> from_fields(const std::vector<std::string> &r3_fields,
                                                     const std::vector<std::string> *r2_fields = nullptr) {
    if (r3_fields.size() <= 8) {
      return std::nullopt;
    }

    ControllerStatus status;
    status.software_version = r3_fields[5];
    status.model = r3_fields[6];
    status.serial_number_1 = r3_fields[7];
    status.serial_number_2 = r3_fields[8];

    if (r2_fields != nullptr && r2_fields->size() > 10) {
      auto year = parse_int_((*r2_fields)[10]);
      auto month = parse_int_((*r2_fields)[9]);
      auto day = parse_int_((*r2_fields)[8]);
      auto hour = parse_int_((*r2_fields)[5]);
      auto minute = parse_int_((*r2_fields)[6]);
      auto second = parse_int_((*r2_fields)[7]);

      if (year.has_value() && month.has_value() && day.has_value() && hour.has_value() && minute.has_value() &&
          second.has_value()) {
        auto epoch = to_epoch_utc_seconds_(year.value(), month.value(), day.value(), hour.value(), minute.value(),
                                           second.value());
        if (epoch.has_value()) {
          status.controller_epoch = epoch;
        }
      }
    }

    return status;
  }

 private:
  static std::optional<int> parse_int_(const std::string &value) {
    if (value.empty()) {
      return std::nullopt;
    }

    errno = 0;
    char *end = nullptr;
    long out = std::strtol(value.c_str(), &end, 10);

    if (errno != 0 || end == nullptr || *end != '\0') {
      return std::nullopt;
    }

    if (out < std::numeric_limits<int>::min() || out > std::numeric_limits<int>::max()) {
      return std::nullopt;
    }

    return static_cast<int>(out);
  }

  static bool is_leap_year_(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
  }

  static std::optional<int64_t> to_epoch_utc_seconds_(int year, int month, int day, int hour, int minute,
                                                       int second) {
    if (year < 1970 || month < 1 || month > 12 || hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 ||
        second > 59) {
      return std::nullopt;
    }

    static const int month_lengths[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int max_day = month_lengths[month - 1];
    if (month == 2 && is_leap_year_(year)) {
      max_day = 29;
    }
    if (day < 1 || day > max_day) {
      return std::nullopt;
    }

    // Days from civil date to Unix epoch using a timezone-independent algorithm.
    int y = year;
    unsigned m = static_cast<unsigned>(month);
    unsigned d = static_cast<unsigned>(day);
    y -= m <= 2;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? static_cast<unsigned>(-3) : static_cast<unsigned>(9))) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const int64_t days = static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(doe) - 719468;

    return days * 86400 + static_cast<int64_t>(hour) * 3600 + static_cast<int64_t>(minute) * 60 + second;
  }
};

// RfRegisterStore accumulates latest raw fields by register label.
// It does not parse lines; callers must pass parsed (label, fields).
class RfRegisterStore {
 public:
  bool update_fields(const std::string &label, std::vector<std::string> fields) {
    if (label.empty()) {
      return false;
    }

    this->registers_[label] = std::move(fields);
    return true;
  }

  std::optional<AnyRegisterLine> typed_register(const std::string &label) const {
    auto it = registers_.find(label);
    if (it == registers_.end()) {
      return std::nullopt;
    }

    return decode_register_(label, it->second);
  }

  void for_each_typed_register(const std::function<void(const AnyRegisterLine &)> &visitor) const {
    for (const auto &[label, fields] : registers_) {
      auto decoded = decode_register_(label, fields);
      if (decoded.has_value()) {
        visitor(decoded.value());
      }
    }
  }

  std::optional<std::reference_wrapper<const std::vector<std::string>>> fields_for(const std::string &label) const {
    auto it = registers_.find(label);
    if (it == registers_.end()) {
      return std::nullopt;
    }

    return std::cref(it->second);
  }

 private:
  static std::optional<AnyRegisterLine> decode_register_(const std::string &label,
                                                         const std::vector<std::string> &fields) {
    using Decoder = std::optional<AnyRegisterLine> (*)(const std::vector<std::string> &);

    static const std::map<std::string, Decoder> decoders = {
        {RegisterR2::kLabel,
         [](const std::vector<std::string> &raw) -> std::optional<AnyRegisterLine> {
           auto parsed = RegisterR2::from_fields(raw);
           if (!parsed.has_value()) {
             return std::nullopt;
           }
           return AnyRegisterLine{std::move(parsed.value())};
         }},
        {RegisterR3::kLabel,
         [](const std::vector<std::string> &raw) -> std::optional<AnyRegisterLine> {
           auto parsed = RegisterR3::from_fields(raw);
           if (!parsed.has_value()) {
             return std::nullopt;
           }
           return AnyRegisterLine{std::move(parsed.value())};
         }},
        {RegisterR4::kLabel,
         [](const std::vector<std::string> &raw) -> std::optional<AnyRegisterLine> {
           auto parsed = RegisterR4::from_fields(raw);
           if (!parsed.has_value()) {
             return std::nullopt;
           }
           return AnyRegisterLine{std::move(parsed.value())};
         }},
        {RegisterR5::kLabel,
         [](const std::vector<std::string> &raw) -> std::optional<AnyRegisterLine> {
           auto parsed = RegisterR5::from_fields(raw);
           if (!parsed.has_value()) {
             return std::nullopt;
           }
           return AnyRegisterLine{std::move(parsed.value())};
         }},
        {RegisterR6::kLabel,
         [](const std::vector<std::string> &raw) -> std::optional<AnyRegisterLine> {
           auto parsed = RegisterR6::from_fields(raw);
           if (!parsed.has_value()) {
             return std::nullopt;
           }
           return AnyRegisterLine{std::move(parsed.value())};
         }},
        {RegisterR7::kLabel,
         [](const std::vector<std::string> &raw) -> std::optional<AnyRegisterLine> {
           auto parsed = RegisterR7::from_fields(raw);
           if (!parsed.has_value()) {
             return std::nullopt;
           }
           return AnyRegisterLine{std::move(parsed.value())};
         }},
        {RegisterR9::kLabel,
         [](const std::vector<std::string> &raw) -> std::optional<AnyRegisterLine> {
           auto parsed = RegisterR9::from_fields(raw);
           if (!parsed.has_value()) {
             return std::nullopt;
           }
           return AnyRegisterLine{std::move(parsed.value())};
         }},
        {RegisterRA::kLabel,
         [](const std::vector<std::string> &raw) -> std::optional<AnyRegisterLine> {
           auto parsed = RegisterRA::from_fields(raw);
           if (!parsed.has_value()) {
             return std::nullopt;
           }
           return AnyRegisterLine{std::move(parsed.value())};
         }},
        {RegisterRB::kLabel,
         [](const std::vector<std::string> &raw) -> std::optional<AnyRegisterLine> {
           auto parsed = RegisterRB::from_fields(raw);
           if (!parsed.has_value()) {
             return std::nullopt;
           }
           return AnyRegisterLine{std::move(parsed.value())};
         }},
        {RegisterRC::kLabel,
         [](const std::vector<std::string> &raw) -> std::optional<AnyRegisterLine> {
           auto parsed = RegisterRC::from_fields(raw);
           if (!parsed.has_value()) {
             return std::nullopt;
           }
           return AnyRegisterLine{std::move(parsed.value())};
         }},
        {RegisterRE::kLabel,
         [](const std::vector<std::string> &raw) -> std::optional<AnyRegisterLine> {
           auto parsed = RegisterRE::from_fields(raw);
           if (!parsed.has_value()) {
             return std::nullopt;
           }
           return AnyRegisterLine{std::move(parsed.value())};
         }},
        {RegisterRG::kLabel,
         [](const std::vector<std::string> &raw) -> std::optional<AnyRegisterLine> {
           auto parsed = RegisterRG::from_fields(raw);
           if (!parsed.has_value()) {
             return std::nullopt;
           }
           return AnyRegisterLine{std::move(parsed.value())};
         }},
    };

    auto decoder = decoders.find(label);
    if (decoder == decoders.end()) {
      return AnyRegisterLine{UnknownRegisterLine{label, fields}};
    }

    return decoder->second(fields);
  }

  std::map<std::string, std::vector<std::string>> registers_;
};

struct SpaNetState {
  ControllerStatus controller;

  bool recompute_from(const RfRegisterStore &store) {
    auto r3 = store.fields_for("R3");
    if (!r3.has_value()) {
      this->controller = ControllerStatus{};
      return true;
    }

    auto r2 = store.fields_for("R2");
    const auto *r2_fields = r2.has_value() ? &r2->get() : nullptr;

    auto status = ControllerStatus::from_fields(r3->get(), r2_fields);
    if (!status.has_value()) {
      return false;
    }

    this->controller = std::move(status.value());
    return true;
  }
};

}  // namespace esphome::spanet
