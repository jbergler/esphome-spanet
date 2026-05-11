#include "spanet_state.h"

#include <variant>

namespace esphome::spanet {

RegisterStore::RegisterStore() {
  this->state = State{
      .controller_status = ControllerStatus{},
  };
}

const State &RegisterStore::get_state() const { return this->state; }

bool RegisterStore::update(const std::string &line) {
  auto parsed = SpaNetParser::parse_register_line(line);
  if (!parsed.has_value()) {
    return false;
  }
  if (!this->registers_.update(parsed.value())) {
    return false;
  }
  this->update_controller_status();
  return true;
}

bool Registers::update(const AnyRegisterLine &line) {
  return std::visit(StoreRegisterVisitor{this}, line);
}

const Registers &RegisterStore::get_registers() const { return this->registers_; }

void RegisterStore::update_controller_status() {
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

}  // namespace esphome::spanet
