#pragma once

#include <cctype>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "spanet_state.h"

namespace esphome::spanet {

enum class MessageType {
  kStateUpdate,
  kAck,
  kUnknown,
};

struct RegisterR2 {
  static constexpr const char *kLabel = "R2";

  std::string mains_current;
  std::string mains_voltage;
  std::string case_temperature;
  std::string port_current;
  std::string spa_day_of_week;
  std::string spa_time_hour;
  std::string spa_time_minute;
  std::string spa_time_second;
  std::string spa_time_day;
  std::string spa_time_month;
  std::string spa_time_year;
  std::string heater_temperature;
  std::string pool_temperature;
  std::string water_present;
  std::string unknown_15;
  std::string awake_minutes_remaining;
  std::string filt_pump_run_time_total;
  std::string filt_pump_req_mins;
  std::string load_timeout;
  std::string hour_meter;
  std::string relay_1;
  std::string relay_2;
  std::string relay_3;
  std::string relay_4;
  std::string relay_5;
  std::string relay_6;
  std::string relay_7;
  std::string relay_8;
  std::string relay_9;

  static std::optional<RegisterR2> from_fields(const std::vector<std::string> &fields) {
    if (fields.size() <= 28) {
      return std::nullopt;
    }

    RegisterR2 out;
    out.mains_current = fields[0];
    out.mains_voltage = fields[1];
    out.case_temperature = fields[2];
    out.port_current = fields[3];
    out.spa_day_of_week = fields[4];
    out.spa_time_hour = fields[5];
    out.spa_time_minute = fields[6];
    out.spa_time_second = fields[7];
    out.spa_time_day = fields[8];
    out.spa_time_month = fields[9];
    out.spa_time_year = fields[10];
    out.heater_temperature = fields[11];
    out.pool_temperature = fields[12];
    out.water_present = fields[13];
    out.unknown_15 = fields[14];
    out.awake_minutes_remaining = fields[15];
    out.filt_pump_run_time_total = fields[16];
    out.filt_pump_req_mins = fields[17];
    out.load_timeout = fields[18];
    out.hour_meter = fields[19];
    out.relay_1 = fields[20];
    out.relay_2 = fields[21];
    out.relay_3 = fields[22];
    out.relay_4 = fields[23];
    out.relay_5 = fields[24];
    out.relay_6 = fields[25];
    out.relay_7 = fields[26];
    out.relay_8 = fields[27];
    out.relay_9 = fields[28];

    return out;
  }
};

struct RegisterR3 {
  static constexpr const char *kLabel = "R3";
  static constexpr size_t kSoftwareVersionIndex = 5;
  static constexpr size_t kModelIndex = 6;
  static constexpr size_t kSerial1Index = 7;
  static constexpr size_t kSerial2Index = 8;

  std::string software_version;
  std::string model;
  std::string serial_number_1;
  std::string serial_number_2;

  static std::optional<RegisterR3> from_fields(const std::vector<std::string> &fields) {
    if (fields.size() <= kSerial2Index) {
      return std::nullopt;
    }

    RegisterR3 out;
    out.software_version = fields[kSoftwareVersionIndex];
    out.model = fields[kModelIndex];
    out.serial_number_1 = fields[kSerial1Index];
    out.serial_number_2 = fields[kSerial2Index];
    return out;
  }
};

struct RegisterR4 {
  static constexpr const char *kLabel = "R4";

  std::string mode;
  std::string ser1_timer;
  std::string ser2_timer;
  std::string ser3_timer;
  std::string heat_mode;
  std::string pump_idle_timer;
  std::string pump_run_timer;
  std::string adt_pool_hys;
  std::string adt_heater_hys;
  std::string power;
  std::string power_kwh;
  std::string power_today;
  std::string power_yesterday;
  std::string thermal_cut_out;
  std::string test_d1;
  std::string test_d2;
  std::string test_d3;
  std::string element_heat_source_offset;
  std::string frequency;
  std::string hp_heat_source_offset_heat;
  std::string hp_heat_source_offset_cool;
  std::string heat_source_off_time;
  std::string vari_mode;
  std::string vari_speed;
  std::string vari_percent;

  static std::optional<RegisterR4> from_fields(const std::vector<std::string> &fields) {
    if (fields.size() <= 24) {
      return std::nullopt;
    }

    RegisterR4 out;
    out.mode = fields[0];
    out.ser1_timer = fields[1];
    out.ser2_timer = fields[2];
    out.ser3_timer = fields[3];
    out.heat_mode = fields[4];
    out.pump_idle_timer = fields[5];
    out.pump_run_timer = fields[6];
    out.adt_pool_hys = fields[7];
    out.adt_heater_hys = fields[8];
    out.power = fields[9];
    out.power_kwh = fields[10];
    out.power_today = fields[11];
    out.power_yesterday = fields[12];
    out.thermal_cut_out = fields[13];
    out.test_d1 = fields[14];
    out.test_d2 = fields[15];
    out.test_d3 = fields[16];
    out.element_heat_source_offset = fields[17];
    out.frequency = fields[18];
    out.hp_heat_source_offset_heat = fields[19];
    out.hp_heat_source_offset_cool = fields[20];
    out.heat_source_off_time = fields[21];
    out.vari_mode = fields[22];
    out.vari_speed = fields[23];
    out.vari_percent = fields[24];
    return out;
  }
};

struct RegisterR5 {
  static constexpr const char *kLabel = "R5";

  std::string status_0;
  std::string status_1;
  std::string status_2;
  std::string status_3;
  std::string status_4;
  std::string status_5;
  std::string status_6;
  std::string status_7;
  std::string status_8;
  std::string status_9;
  std::string status_10;
  std::string status_11;
  std::string status_12;
  std::string status_13;
  std::string status_14;
  std::string status_15;
  std::string status_16;
  std::string status_17;
  std::string status_18;
  std::string status_19;
  std::string status_20;
  std::string status_21;
  std::string status_22;
  std::string status_23;
  std::string status_24;
  std::string status_25;

  static std::optional<RegisterR5> from_fields(const std::vector<std::string> &fields) {
    if (fields.size() <= 25) {
      return std::nullopt;
    }

    RegisterR5 out;
    out.status_0 = fields[0];
    out.status_1 = fields[1];
    out.status_2 = fields[2];
    out.status_3 = fields[3];
    out.status_4 = fields[4];
    out.status_5 = fields[5];
    out.status_6 = fields[6];
    out.status_7 = fields[7];
    out.status_8 = fields[8];
    out.status_9 = fields[9];
    out.status_10 = fields[10];
    out.status_11 = fields[11];
    out.status_12 = fields[12];
    out.status_13 = fields[13];
    out.status_14 = fields[14];
    out.status_15 = fields[15];
    out.status_16 = fields[16];
    out.status_17 = fields[17];
    out.status_18 = fields[18];
    out.status_19 = fields[19];
    out.status_20 = fields[20];
    out.status_21 = fields[21];
    out.status_22 = fields[22];
    out.status_23 = fields[23];
    out.status_24 = fields[24];
    out.status_25 = fields[25];
    return out;
  }
};

struct RegisterR6 {
  static constexpr const char *kLabel = "R6";

  std::string clean_cycle;
  std::string vari_value;
  std::string brightness;
  std::string current_color;
  std::string color_mode;
  std::string light_effect_speed;
  std::string filt_set_hrs;
  std::string filt_block_hrs;
  std::string set_temperature;
  std::string l_24hours;
  std::string power_save_level;
  std::string peak_power_begin;
  std::string peak_power_end;
  std::string sleep_timer_1_day;
  std::string sleep_timer_2_day;
  std::string sleep_timer_1_begin;
  std::string sleep_timer_2_begin;
  std::string sleep_timer_1_end;
  std::string sleep_timer_2_end;
  std::string default_screen;
  std::string timeout;
  std::string variable_pump;
  std::string hifi;
  std::string brand;
  std::string prime;
  std::string element;
  std::string type;
  std::string gas;

  static std::optional<RegisterR6> from_fields(const std::vector<std::string> &fields) {
    if (fields.size() <= 27) {
      return std::nullopt;
    }

    RegisterR6 out;
    out.clean_cycle = fields[0];
    out.vari_value = fields[1];
    out.brightness = fields[2];
    out.current_color = fields[3];
    out.color_mode = fields[4];
    out.light_effect_speed = fields[5];
    out.filt_set_hrs = fields[6];
    out.filt_block_hrs = fields[7];
    out.set_temperature = fields[8];
    out.l_24hours = fields[9];
    out.power_save_level = fields[10];
    out.peak_power_begin = fields[11];
    out.peak_power_end = fields[12];
    out.sleep_timer_1_day = fields[13];
    out.sleep_timer_2_day = fields[14];
    out.sleep_timer_1_begin = fields[15];
    out.sleep_timer_2_begin = fields[16];
    out.sleep_timer_1_end = fields[17];
    out.sleep_timer_2_end = fields[18];
    out.default_screen = fields[19];
    out.timeout = fields[20];
    out.variable_pump = fields[21];
    out.hifi = fields[22];
    out.brand = fields[23];
    out.prime = fields[24];
    out.element = fields[25];
    out.type = fields[26];
    out.gas = fields[27];
    return out;
  }
};

struct RegisterR7 {
  static constexpr const char *kLabel = "R7";

  std::string wcln_time;
  std::string temperature_units;
  std::string ozone_off;
  std::string ozone_24hrs;
  std::string circulation_24hrs;
  std::string cjet;
  std::string vele;
  std::string v_max;
  std::string v_min;
  std::string v_max_24hrs;
  std::string v_min_24hrs;
  std::string current_zero;
  std::string current_adjust;
  std::string voltage_adjust;
  std::string ser_1;
  std::string ser_2;
  std::string ser_3;
  std::string vmax;
  std::string ahys;
  std::string huse;
  std::string hele;
  std::string hpmp;
  std::string pmin;
  std::string pflt;
  std::string phtr;
  std::string pmax;
  std::string unknown_26;
  std::string unknown_27;
  std::string unknown_28;
  std::string unknown_29;
  std::string unknown_30;

  static std::optional<RegisterR7> from_fields(const std::vector<std::string> &fields) {
    if (fields.size() <= 30) {
      return std::nullopt;
    }

    RegisterR7 out;
    out.wcln_time = fields[0];
    out.temperature_units = fields[1];
    out.ozone_off = fields[2];
    out.ozone_24hrs = fields[3];
    out.circulation_24hrs = fields[4];
    out.cjet = fields[5];
    out.vele = fields[6];
    out.v_max = fields[7];
    out.v_min = fields[8];
    out.v_max_24hrs = fields[9];
    out.v_min_24hrs = fields[10];
    out.current_zero = fields[11];
    out.current_adjust = fields[12];
    out.voltage_adjust = fields[13];
    out.ser_1 = fields[14];
    out.ser_2 = fields[15];
    out.ser_3 = fields[16];
    out.vmax = fields[17];
    out.ahys = fields[18];
    out.huse = fields[19];
    out.hele = fields[20];
    out.hpmp = fields[21];
    out.pmin = fields[22];
    out.pflt = fields[23];
    out.phtr = fields[24];
    out.pmax = fields[25];
    out.unknown_26 = fields[26];
    out.unknown_27 = fields[27];
    out.unknown_28 = fields[28];
    out.unknown_29 = fields[29];
    out.unknown_30 = fields[30];
    return out;
  }
};

struct RegisterR9 {
  static constexpr const char *kLabel = "R9";

  std::string fault_code;
  std::string accum_1;
  std::string accum_2;
  std::string accum_3;
  std::string accum_4;
  std::string accum_5;
  std::string accum_6;
  std::string accum_7;
  std::string accum_8;
  std::string accum_9;
  std::string accum_10;
  std::string accum_11;

  static std::optional<RegisterR9> from_fields(const std::vector<std::string> &fields) {
    if (fields.size() <= 11) {
      return std::nullopt;
    }

    RegisterR9 out;
    out.fault_code = fields[0];
    out.accum_1 = fields[1];
    out.accum_2 = fields[2];
    out.accum_3 = fields[3];
    out.accum_4 = fields[4];
    out.accum_5 = fields[5];
    out.accum_6 = fields[6];
    out.accum_7 = fields[7];
    out.accum_8 = fields[8];
    out.accum_9 = fields[9];
    out.accum_10 = fields[10];
    out.accum_11 = fields[11];
    return out;
  }
};

struct RegisterRA {
  static constexpr const char *kLabel = "RA";

  std::string fault_code;
  std::string accum_1;
  std::string accum_2;
  std::string accum_3;
  std::string accum_4;
  std::string accum_5;
  std::string accum_6;
  std::string accum_7;
  std::string accum_8;
  std::string accum_9;
  std::string accum_10;
  std::string accum_11;

  static std::optional<RegisterRA> from_fields(const std::vector<std::string> &fields) {
    if (fields.size() <= 11) {
      return std::nullopt;
    }

    RegisterRA out;
    out.fault_code = fields[0];
    out.accum_1 = fields[1];
    out.accum_2 = fields[2];
    out.accum_3 = fields[3];
    out.accum_4 = fields[4];
    out.accum_5 = fields[5];
    out.accum_6 = fields[6];
    out.accum_7 = fields[7];
    out.accum_8 = fields[8];
    out.accum_9 = fields[9];
    out.accum_10 = fields[10];
    out.accum_11 = fields[11];
    return out;
  }
};

struct RegisterRB {
  static constexpr const char *kLabel = "RB";

  std::string fault_code;
  std::string accum_1;
  std::string accum_2;
  std::string accum_3;
  std::string accum_4;
  std::string accum_5;
  std::string accum_6;
  std::string accum_7;
  std::string accum_8;
  std::string accum_9;
  std::string accum_10;
  std::string accum_11;

  static std::optional<RegisterRB> from_fields(const std::vector<std::string> &fields) {
    if (fields.size() <= 11) {
      return std::nullopt;
    }

    RegisterRB out;
    out.fault_code = fields[0];
    out.accum_1 = fields[1];
    out.accum_2 = fields[2];
    out.accum_3 = fields[3];
    out.accum_4 = fields[4];
    out.accum_5 = fields[5];
    out.accum_6 = fields[6];
    out.accum_7 = fields[7];
    out.accum_8 = fields[8];
    out.accum_9 = fields[9];
    out.accum_10 = fields[10];
    out.accum_11 = fields[11];
    return out;
  }
};

struct RegisterRC {
  static constexpr const char *kLabel = "RC";

  std::string outlet_0;
  std::string outlet_1;
  std::string outlet_2;
  std::string outlet_3;
  std::string outlet_4;
  std::string outlet_5;
  std::string outlet_6;
  std::string outlet_7;
  std::string outlet_8;
  std::string outlet_9;
  std::string outlet_10;
  std::string outlet_11;
  std::string outlet_12;
  std::string outlet_13;

  static std::optional<RegisterRC> from_fields(const std::vector<std::string> &fields) {
    if (fields.size() <= 13) {
      return std::nullopt;
    }

    RegisterRC out;
    out.outlet_0 = fields[0];
    out.outlet_1 = fields[1];
    out.outlet_2 = fields[2];
    out.outlet_3 = fields[3];
    out.outlet_4 = fields[4];
    out.outlet_5 = fields[5];
    out.outlet_6 = fields[6];
    out.outlet_7 = fields[7];
    out.outlet_8 = fields[8];
    out.outlet_9 = fields[9];
    out.outlet_10 = fields[10];
    out.outlet_11 = fields[11];
    out.outlet_12 = fields[12];
    out.outlet_13 = fields[13];
    return out;
  }
};

struct RegisterRE {
  static constexpr const char *kLabel = "RE";

  std::string param_0;
  std::string param_1;
  std::string param_2;
  std::string param_3;
  std::string param_4;
  std::string param_5;
  std::string param_6;
  std::string param_7;
  std::string param_8;
  std::string param_9;
  std::string param_10;
  std::string param_11;
  std::string param_12;
  std::string param_13;
  std::string param_14;
  std::string param_15;
  std::string param_16;
  std::string param_17;
  std::string param_18;
  std::string param_19;
  std::string param_20;
  std::string param_21;
  std::string param_22;
  std::string param_23;
  std::string param_24;
  std::string param_25;
  std::string param_26;
  std::string param_27;
  std::string param_28;
  std::string param_29;

  static std::optional<RegisterRE> from_fields(const std::vector<std::string> &fields) {
    if (fields.size() <= 29) {
      return std::nullopt;
    }

    RegisterRE out;
    out.param_0 = fields[0];
    out.param_1 = fields[1];
    out.param_2 = fields[2];
    out.param_3 = fields[3];
    out.param_4 = fields[4];
    out.param_5 = fields[5];
    out.param_6 = fields[6];
    out.param_7 = fields[7];
    out.param_8 = fields[8];
    out.param_9 = fields[9];
    out.param_10 = fields[10];
    out.param_11 = fields[11];
    out.param_12 = fields[12];
    out.param_13 = fields[13];
    out.param_14 = fields[14];
    out.param_15 = fields[15];
    out.param_16 = fields[16];
    out.param_17 = fields[17];
    out.param_18 = fields[18];
    out.param_19 = fields[19];
    out.param_20 = fields[20];
    out.param_21 = fields[21];
    out.param_22 = fields[22];
    out.param_23 = fields[23];
    out.param_24 = fields[24];
    out.param_25 = fields[25];
    out.param_26 = fields[26];
    out.param_27 = fields[27];
    out.param_28 = fields[28];
    out.param_29 = fields[29];
    return out;
  }
};

struct RegisterRG {
  static constexpr const char *kLabel = "RG";

  std::string pump_1;
  std::string pump_2;
  std::string pump_3;
  std::string pump_4;
  std::string pump_5;
  std::string pump_6;
  std::string pump_7;
  std::string pump_8;
  std::string pump_9;
  std::string pump_10;
  std::string pump_11;
  std::string pump_12;
  std::string pump_13;
  std::string pump_14;

  static std::optional<RegisterRG> from_fields(const std::vector<std::string> &fields) {
    if (fields.size() <= 14) {
      return std::nullopt;
    }

    RegisterRG out;
    out.pump_1 = fields[0];
    out.pump_2 = fields[1];
    out.pump_3 = fields[2];
    out.pump_4 = fields[3];
    out.pump_5 = fields[4];
    out.pump_6 = fields[5];
    out.pump_7 = fields[6];
    out.pump_8 = fields[7];
    out.pump_9 = fields[8];
    out.pump_10 = fields[9];
    out.pump_11 = fields[10];
    out.pump_12 = fields[11];
    out.pump_13 = fields[12];
    out.pump_14 = fields[14];
    return out;
  }
};

// Fallback for register labels that are not yet modelled with a dedicated
// typed struct.
struct UnknownRegisterLine {
  std::string label;
  std::vector<std::string> fields;
};

using AnyRegisterLine = std::variant<RegisterR2, RegisterR3, RegisterR4, RegisterR5, RegisterR6, RegisterR7, RegisterR9, RegisterRA, RegisterRB, RegisterRC, RegisterRE, RegisterRG, UnknownRegisterLine>;

// SpaNetParser is stateless. Every method is a pure function of its inputs.
class SpaNetParser {
 public:
  // Determines the routing of a single UART line (no embedded newlines).
  //   kStateUpdate → feed into RfRegisterStore::update()
  //   kAck         → match against a pending command expectation
  //   kUnknown     → log and discard
  //
  // Both the RF start line ("RF:,R2:...") and standalone register continuation
  // lines ("R3:...", "RA:...") are classified as kStateUpdate, so the store
  // can be fed directly without any multi-line assembly step.
  static MessageType classify_message(const std::string &line) {
    auto trimmed = trim_(line);
    if (trimmed.rfind("RF:", 0) == 0 || trimmed.rfind("RF,", 0) == 0) {
      return MessageType::kStateUpdate;
    }

    auto colon_pos = trimmed.find(':');
    if (colon_pos != std::string::npos && is_register_label_(trimmed.substr(0, colon_pos))) {
      return MessageType::kStateUpdate;
    }

    if (!trimmed.empty() && (trimmed[0] == 'S' || trimmed[0] == 'W')) {
      return MessageType::kAck;
    }

    return MessageType::kUnknown;
  }

  // Parses one kStateUpdate UART line into a (register_label, fields) pair.
  // Returns nullopt if the line does not contain a recognisable register pattern.
  //
  // Handles both line shapes present in real SpaNET RF responses:
  //   RF start line:   "RF:,R2:1,2,3"  → ("R2", ["1","2","3"])
  //   Continuation:    "R3:10,20,30"   → ("R3", ["10","20","30"])
  static std::optional<std::pair<std::string, std::vector<std::string>>> parse_register_line(
      const std::string &line) {
    auto content = trim_(line);

    // Strip "RF:" prefix (but keep leading comma from the first response line)
    if (content.rfind("RF:", 0) == 0) {
      content = content.substr(3);
    }

    // Handle real controller format: ,{LABEL},{fields},:
    if (!content.empty() && content[0] == ',') {
      content = content.substr(1);  // Strip leading comma

      // Find the trailing : or :*
      auto colon_pos = content.find(':');
      if (colon_pos == std::string::npos) {
        return std::nullopt;
      }

      // Extract everything before the colon and split by comma
      auto all_fields = split_by_comma_(content.substr(0, colon_pos));
      if (all_fields.empty()) {
        return std::nullopt;
      }

      // Remove trailing empty fields (from trailing comma)
      while (!all_fields.empty() && all_fields.back().empty()) {
        all_fields.pop_back();
      }

      if (all_fields.empty()) {
        return std::nullopt;
      }

      auto label = all_fields[0];
      if (!is_register_label_(label)) {
        return std::nullopt;
      }

      // Remove the label and keep only the data fields
      all_fields.erase(all_fields.begin());
      return std::make_pair(label, std::move(all_fields));
    }

    // Legacy format: {LABEL}:{fields}
    auto colon_pos = content.find(':');
    if (colon_pos == std::string::npos) {
      return std::nullopt;
    }

    auto label = content.substr(0, colon_pos);
    if (!is_register_label_(label)) {
      return std::nullopt;
    }

    auto fields = split_by_comma_(content.substr(colon_pos + 1));
    return std::make_pair(label, std::move(fields));
  }

 private:
  static std::string trim_(std::string value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
      value.erase(value.begin());
    }

    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back()))) {
      value.pop_back();
    }

    return value;
  }

  static bool is_register_label_(const std::string &token) {
    if (token.size() < 2 || token[0] != 'R') {
      return false;
    }

    for (size_t index = 1; index < token.size(); index++) {
      if (!std::isalnum(static_cast<unsigned char>(token[index]))) {
        return false;
      }
    }

    return true;
  }

  // Splits the comma-separated field list of a single register line.
  static std::vector<std::string> split_by_comma_(const std::string &s) {
    std::vector<std::string> fields;
    std::string current;

    for (char ch : s) {
      if (ch == ',') {
        fields.push_back(trim_(current));
        current.clear();
      } else {
        current.push_back(ch);
      }
    }

    fields.push_back(trim_(current));
    return fields;
  }
};

// RfRegisterStore accumulates the latest known fields for each register label,
// updated one UART line at a time. It never resets: each call to update()
// overwrites only the entry for the register found on that line, leaving all
// other registers untouched.
//
// This makes the store resilient to partial or interrupted RF responses.
// If only R3 has been received, controller identity is already accessible
// even though R2, R4, and the rest have not yet arrived.
//
// Extending the store for new typed accessors:
//   1. Add fields to ControllerIdentity or SpaNetState for the new data.
//   2. Add a new accessor method (e.g. water_temperature()) that reads from
//      registers_ using the appropriate label and field mappings.
//   3. Cover the accessor with a focused unit test before wiring it in.
//   4. Do not add parsing logic here — parsing belongs in SpaNetParser.
class RfRegisterStore {
 public:
  // Feed one kStateUpdate UART line. The register entry for the label found on
  // this line is replaced atomically with the new field list. Returns true if a
  // valid register was parsed and stored.
  bool update(const std::string &line) {
    auto parsed = SpaNetParser::parse_register_line(line);
    if (!parsed.has_value()) {
      return false;
    }

    registers_[parsed->first] = std::move(parsed->second);
    return true;
  }

  // Returns typed controller identity extracted from the latest R3 data.
  // Returns nullopt if R3 has not been received yet, or does not have enough
  // fields for the given layout.
  std::optional<ControllerIdentity> controller_identity() const {
    auto register_line = this->typed_register(RegisterR3::kLabel);
    if (!register_line.has_value()) {
      return std::nullopt;
    }

    const auto &typed = register_line.value();
    if (!std::holds_alternative<RegisterR3>(typed)) {
      return std::nullopt;
    }

    const auto &r3 = std::get<RegisterR3>(typed);
    ControllerIdentity identity;
    identity.software_version = r3.software_version;
    identity.model = r3.model;
    identity.serial_number_1 = r3.serial_number_1;
    identity.serial_number_2 = r3.serial_number_2;
    return identity;
  }

  // Returns one decoded register line as a typed variant.
  std::optional<AnyRegisterLine> typed_register(const std::string &label) const {
    auto it = registers_.find(label);
    if (it == registers_.end()) {
      return std::nullopt;
    }

    return decode_register_(label, it->second);
  }

  // Visits all currently stored register lines with typed decoding.
  void for_each_typed_register(const std::function<void(const AnyRegisterLine &)> &visitor) const {
    for (const auto &[label, fields] : registers_) {
      auto decoded = decode_register_(label, fields);
      if (decoded.has_value()) {
        visitor(decoded.value());
      }
    }
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

  // Latest parsed fields per register label. Entries persist until overwritten
  // by a later line carrying the same label.
  std::map<std::string, std::vector<std::string>> registers_;
};

}  // namespace esphome::spanet
