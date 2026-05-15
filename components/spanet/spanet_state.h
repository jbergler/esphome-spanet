#pragma once

#include <array>
#include <ctime>
#include <optional>
#include <string>
#include <variant>

#include "spanet_parser.h"

namespace esphome::spanet {

struct ControllerStatus {
  std::string software_version;
  int major_version{0};
  std::string model;
  std::string serial_number;
  std::optional<time_t> current_time;
};

struct TemperatureStatus {
  std::optional<float> water_c;
  std::optional<float> setpoint_c;
  std::optional<float> heater_c;
  std::optional<float> case_c;
};

struct PowerStatus {
  std::optional<float> mains_voltage_v;
  std::optional<float> mains_current_a;
  std::optional<float> instant_power_w;
  std::optional<float> total_energy_kwh;
};

struct ClimateStatus {
  std::optional<bool> heating_active;
};

struct LightStatus {
  bool is_on{false};
  uint8_t brightness{1};    // Device scale: 1-5
  uint8_t color_index{0};   // Device scale: 0-31
  uint8_t effect_mode{0};   // Device scale: 0-4 (white/colour/step/fade/party)
  uint8_t effect_speed{1};  // Device scale: 1-5
};

struct PumpStatus {
  bool installed{false};
  bool capabilities_valid{false};
  int speed_type{0};
  std::array<bool, 5> supports_raw_mode{{false, false, false, false, false}};
  std::array<int, 3> manual_raw_modes{{0, 0, 0}};
  size_t manual_raw_mode_count{0};
  bool supports_speed{false};
  bool supports_auto{false};

  std::optional<int> current_raw_mode;
  bool is_on{false};
  bool auto_mode_active{false};
};

struct State {
  ControllerStatus controller_status;
  TemperatureStatus temperatures;
  PowerStatus power;
  ClimateStatus climate;
  LightStatus light;
  std::array<PumpStatus, 5> pumps;
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

  bool update(const AnyRegisterLine &line);
};

struct StoreRegisterVisitor {
  Registers *target;

  bool operator()(const RegisterR2 &reg) {
    target->r2 = reg;
    return true;
  }
  bool operator()(const RegisterR3 &reg) {
    target->r3 = reg;
    return true;
  }
  bool operator()(const RegisterR4 &reg) {
    target->r4 = reg;
    return true;
  }
  bool operator()(const RegisterR5 &reg) {
    target->r5 = reg;
    return true;
  }
  bool operator()(const RegisterR6 &reg) {
    target->r6 = reg;
    return true;
  }
  bool operator()(const RegisterR7 &reg) {
    target->r7 = reg;
    return true;
  }
  bool operator()(const RegisterR9 &reg) {
    target->r9 = reg;
    return true;
  }
  bool operator()(const RegisterRA &reg) {
    target->ra = reg;
    return true;
  }
  bool operator()(const RegisterRB &reg) {
    target->rb = reg;
    return true;
  }
  bool operator()(const RegisterRC &reg) {
    target->rc = reg;
    return true;
  }
  bool operator()(const RegisterRE &reg) {
    target->re = reg;
    return true;
  }
  bool operator()(const RegisterRG &reg) {
    target->rg = reg;
    return true;
  }
  bool operator()(const UnknownRegisterLine &) { return true; }
};

class RegisterStore {
 public:
  RegisterStore();

  bool update(const std::string &line);

  const State &get_state() const;
  const Registers &get_registers() const;

 private:
  State state;
  Registers registers_ = {};

  void update_controller_status();

 public:
  State &get_mutable_state() { return state; }
};

// Version-aware RF completion predicate factory: matches RG for V3+, RE for V2/unknown
inline auto make_rf_completion_predicate(const RegisterStore &store) {
  return [&store](const std::string &line) {
    const auto register_label = SpaNetParser::extract_register_label(line);
    const auto &state = store.get_state();
    const auto &major_version = state.controller_status.major_version;
    return register_label == (major_version >= 3 ? "RG" : "RE");
  };
}

}  // namespace esphome::spanet
