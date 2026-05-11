#include "spanet_state.h"

#include <algorithm>
#include <cstdlib>
#include <variant>

namespace esphome::spanet {

static std::string normalize_software_version(const std::string &version) {
  std::string normalized = version;
  constexpr const char kPrefix[] = "SW ";
  if (normalized.rfind(kPrefix, 0) == 0) {
    normalized.erase(0, sizeof(kPrefix) - 1);
  }
  std::replace(normalized.begin(), normalized.end(), ' ', '.');
  return normalized;
}

static std::optional<float> parse_tenths_celsius(const std::string &raw) {
  const char *start = raw.c_str();
  char *end = nullptr;
  long parsed = std::strtol(start, &end, 10);
  if (start == end || *end != '\0') {
    return std::nullopt;
  }
  return static_cast<float>(parsed) / 10.0f;
}

RegisterStore::RegisterStore() {
  this->state = State{
      .controller_status = ControllerStatus{},
      .temperatures = TemperatureStatus{},
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
    this->state.controller_status.software_version = normalize_software_version(r3.software_version);
    this->state.controller_status.model = r3.model;
    this->state.controller_status.serial_number = r3.serial_number_1 + "-" + r3.serial_number_2;
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

  if (this->registers_.r5.has_value()) {
    const auto &r5 = this->registers_.r5.value();
    this->state.temperatures.water_c = parse_tenths_celsius(r5.status_14);
  }

  if (this->registers_.r6.has_value()) {
    const auto &r6 = this->registers_.r6.value();
    this->state.temperatures.setpoint_c = parse_tenths_celsius(r6.set_temperature);
  }
}

}  // namespace esphome::spanet
