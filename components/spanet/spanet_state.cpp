#include "spanet_state.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <string>
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

static std::optional<float> parse_scaled_float(const std::string &raw, float scale) {
  const char *start = raw.c_str();
  char *end = nullptr;
  long parsed = std::strtol(start, &end, 10);
  if (start == end || *end != '\0') {
    return std::nullopt;
  }
  return static_cast<float>(parsed) / scale;
}

static std::optional<float> parse_integer_float(const std::string &raw) { return parse_scaled_float(raw, 1.0f); }

static std::optional<bool> parse_bool_flag(const std::string &raw) {
  const char *start = raw.c_str();
  char *end = nullptr;
  long parsed = std::strtol(start, &end, 10);
  if (start == end || *end != '\0') {
    return std::nullopt;
  }
  return parsed != 0;
}

static std::optional<int> parse_integer(const std::string &raw) {
  const char *start = raw.c_str();
  char *end = nullptr;
  long parsed = std::strtol(start, &end, 10);
  if (start == end || *end != '\0') {
    return std::nullopt;
  }
  return static_cast<int>(parsed);
}

static const std::string &get_rg_install_state(const RegisterRG &rg, size_t pump_index) {
  switch (pump_index) {
    case 0:
      return rg.pump1_install_state;
    case 1:
      return rg.pump2_install_state;
    case 2:
      return rg.pump3_install_state;
    case 3:
      return rg.pump4_install_state;
    default:
      return rg.pump5_install_state;
  }
}

static const std::string &get_r5_pump_mode(const RegisterR5 &r5, size_t pump_index) {
  switch (pump_index) {
    case 0:
      return r5.status_17;
    case 1:
      return r5.status_18;
    case 2:
      return r5.status_19;
    case 3:
      return r5.status_20;
    default:
      return r5.status_21;
  }
}

static void parse_pump_install_state(const std::string &raw, PumpStatus *target) {
  *target = PumpStatus{};

  const size_t first_dash = raw.find('-');
  const size_t second_dash = raw.rfind('-');
  if (first_dash == std::string::npos || second_dash == std::string::npos || first_dash == second_dash) {
    return;
  }

  const auto installed_part = raw.substr(0, first_dash);
  const auto speed_type_part = raw.substr(first_dash + 1, second_dash - first_dash - 1);
  const auto possible_states_part = raw.substr(second_dash + 1);

  target->installed = installed_part == "1";

  auto speed_type = parse_integer(speed_type_part);
  if (speed_type.has_value()) {
    target->speed_type = speed_type.value();
  }

  if (!target->installed || possible_states_part.empty()) {
    return;
  }

  std::array<bool, 5> supports_raw_mode{{false, false, false, false, false}};
  for (char token : possible_states_part) {
    if (!std::isdigit(static_cast<unsigned char>(token))) {
      continue;
    }
    const int value = token - '0';
    if (value >= 0 && value <= 4) {
      supports_raw_mode[static_cast<size_t>(value)] = true;
    }
  }

  target->supports_raw_mode = supports_raw_mode;
  target->supports_auto = supports_raw_mode[4];

  for (size_t raw_mode = 1; raw_mode <= 3; raw_mode++) {
    if (!supports_raw_mode[raw_mode]) {
      continue;
    }
    target->manual_raw_modes[target->manual_raw_mode_count] = static_cast<int>(raw_mode);
    target->manual_raw_mode_count++;
  }

  target->supports_speed = target->manual_raw_mode_count > 1;
  target->capabilities_valid = true;
}

RegisterStore::RegisterStore() {
  this->state = State{
      .controller_status = ControllerStatus{},
      .temperatures = TemperatureStatus{},
      .power = PowerStatus{},
      .climate = ClimateStatus{},
      .pumps = {},
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

bool Registers::update(const AnyRegisterLine &line) { return std::visit(StoreRegisterVisitor{this}, line); }

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

    this->state.temperatures.heater_c = parse_tenths_celsius(r2.heater_temperature);
    this->state.temperatures.case_c = parse_tenths_celsius(r2.case_temperature);
    this->state.power.mains_voltage_v = parse_integer_float(r2.mains_voltage);
    this->state.power.mains_current_a = parse_scaled_float(r2.mains_current, 10.0f);
  }

  if (this->registers_.r5.has_value()) {
    const auto &r5 = this->registers_.r5.value();
    this->state.temperatures.water_c = parse_tenths_celsius(r5.status_14);
    this->state.climate.heating_active = parse_bool_flag(r5.status_11);
  }

  if (this->registers_.r6.has_value()) {
    const auto &r6 = this->registers_.r6.value();
    this->state.temperatures.setpoint_c = parse_tenths_celsius(r6.set_temperature);
  }

  if (this->registers_.r4.has_value()) {
    const auto &r4 = this->registers_.r4.value();
    this->state.power.instant_power_w = parse_scaled_float(r4.power, 10.0f);
    this->state.power.total_energy_kwh = parse_scaled_float(r4.power_kwh, 100.0f);
  }

  for (size_t i = 0; i < this->state.pumps.size(); i++) {
    if (this->registers_.rg.has_value()) {
      parse_pump_install_state(get_rg_install_state(this->registers_.rg.value(), i), &this->state.pumps[i]);
    }

    if (this->registers_.r5.has_value()) {
      auto maybe_raw_mode = parse_integer(get_r5_pump_mode(this->registers_.r5.value(), i));
      this->state.pumps[i].current_raw_mode = maybe_raw_mode;
      if (maybe_raw_mode.has_value()) {
        this->state.pumps[i].is_on = maybe_raw_mode.value() != 0;
        this->state.pumps[i].auto_mode_active = maybe_raw_mode.value() == 4;
      } else {
        this->state.pumps[i].is_on = false;
        this->state.pumps[i].auto_mode_active = false;
      }
    }
  }
}

}  // namespace esphome::spanet
