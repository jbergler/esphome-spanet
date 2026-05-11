#pragma once

#include <ctime>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "spanet_parser.h"

namespace esphome::spanet {

struct ControllerStatus {
  std::string software_version;
  std::string model;
  std::string serial_number_1;
  std::string serial_number_2;
  std::optional<time_t> current_time;
};

struct State {
  ControllerStatus controller_status;
};

struct Registers {
  std::optional<RegisterR2> r2;
  std::optional<RegisterR3> r3;
  std::optional<RegisterR4> r4;
  std::optional<RegisterR5> r5;
  std::optional<RegisterR6> r6;
  std::optional<RegisterR7> r7;
  std::optional<RegisterR9> r9;
  std::optional<RegisterRA> ra;
  std::optional<RegisterRB> rb;
  std::optional<RegisterRC> rc;
  std::optional<RegisterRE> re;
  std::optional<RegisterRG> rg;

  bool store_register(const AnyRegisterLine &line) {
    if (std::holds_alternative<RegisterR2>(line)) {
      this->r2 = std::get<RegisterR2>(line);
    } else if (std::holds_alternative<RegisterR3>(line)) {
      this->r3 = std::get<RegisterR3>(line);
    } else if (std::holds_alternative<RegisterR4>(line)) {
      this->r4 = std::get<RegisterR4>(line);
    } else if (std::holds_alternative<RegisterR5>(line)) {
      this->r5 = std::get<RegisterR5>(line);
    } else if (std::holds_alternative<RegisterR6>(line)) {
      this->r6 = std::get<RegisterR6>(line);
    } else if (std::holds_alternative<RegisterR7>(line)) {
      this->r7 = std::get<RegisterR7>(line);
    } else if (std::holds_alternative<RegisterR9>(line)) {
      this->r9 = std::get<RegisterR9>(line);
    } else if (std::holds_alternative<RegisterRA>(line)) {
      this->ra = std::get<RegisterRA>(line);
    } else if (std::holds_alternative<RegisterRB>(line)) {
      this->rb = std::get<RegisterRB>(line);
    } else if (std::holds_alternative<RegisterRC>(line)) {
      this->rc = std::get<RegisterRC>(line);
    } else if (std::holds_alternative<RegisterRE>(line)) {
      this->re = std::get<RegisterRE>(line);
    } else if (std::holds_alternative<RegisterRG>(line)) {
      this->rg = std::get<RegisterRG>(line);
    } else if (std::holds_alternative<UnknownRegisterLine>(line)) {
      // Unknown register labels are accepted as non-fatal updates.
      return true;
    } else {
      return false;
    }
    return true;
  }
};

class RegisterStore {
 public:
  RegisterStore() {
    this->state = State{
        .controller_status = ControllerStatus{},
    };
  }

  bool update(const std::string &line) {
    auto parsed = SpaNetParser::parse_register_line(line);
    if (!parsed.has_value()) {
      return false;
    }
    if (!this->registers_.store_register(parsed.value())) {
      return false;
    }
    this->update_controller_status();
    return true;
  }

  const State &get_state() const { return this->state; }

  const Registers &get_registers() const { return this->registers_; }

 private:
  State state;
  Registers registers_ = {};

  void update_controller_status() {
    if (this->registers_.r3.has_value()) {
      const auto &r3 = this->registers_.r3.value();
      this->state.controller_status.software_version = r3.software_version;
      this->state.controller_status.model = r3.model;
      this->state.controller_status.serial_number_1 = r3.serial_number_1;
      this->state.controller_status.serial_number_2 = r3.serial_number_2;
    }

    if (this->registers_.r2.has_value()) {
      const auto &r2 = this->registers_.r2.value();
      std::tm tm = {};
      tm.tm_year = std::stoi(r2.spa_time_year) - 1900;
      tm.tm_mon = std::stoi(r2.spa_time_month) - 1;
      tm.tm_mday = std::stoi(r2.spa_time_day);
      tm.tm_hour = std::stoi(r2.spa_time_hour);
      tm.tm_min = std::stoi(r2.spa_time_minute);
      tm.tm_sec = std::stoi(r2.spa_time_second);
      tm.tm_isdst = -1;
      this->state.controller_status.current_time = std::mktime(&tm);
    }
  }
};

}  // namespace esphome::spanet
