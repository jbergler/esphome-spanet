#pragma once

#include <cctype>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

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
  std::string unknown_28;

  static std::optional<RegisterR6> from_fields(const std::vector<std::string> &fields) {
    if (fields.size() <= 27) {
      return std::nullopt;
    }

    RegisterR6 out;
    out.vari_value = fields[0];
    out.brightness = fields[1];
    out.current_color = fields[2];
    out.color_mode = fields[3];
    out.light_effect_speed = fields[4];
    out.filt_set_hrs = fields[5];
    out.filt_block_hrs = fields[6];
    out.set_temperature = fields[7];
    out.l_24hours = fields[8];
    out.power_save_level = fields[9];
    out.peak_power_begin = fields[10];
    out.peak_power_end = fields[11];
    out.sleep_timer_1_day = fields[12];
    out.sleep_timer_2_day = fields[13];
    out.sleep_timer_1_begin = fields[14];
    out.sleep_timer_2_begin = fields[15];
    out.sleep_timer_1_end = fields[16];
    out.sleep_timer_2_end = fields[17];
    out.default_screen = fields[18];
    out.timeout = fields[19];
    out.variable_pump = fields[20];
    out.hifi = fields[21];
    out.brand = fields[22];
    out.prime = fields[23];
    out.element = fields[24];
    out.type = fields[25];
    out.gas = fields[26];
    out.unknown_28 = fields[27];
    return out;
  }
};

struct RegisterR7 {
  static constexpr const char *kLabel = "R7";

  std::string wcln_time;
  std::string ozone_off;
  std::string temperature_units;
  std::string ozone_24hrs;
  std::string cjet;
  std::string circulation_24hrs;
  std::string vele;
  std::string unknown_8;
  std::string unknown_9;
  std::string unknown_10;
  std::string v_max;
  std::string v_min;
  std::string v_max_24hrs;
  std::string v_min_24hrs;
  std::string current_zero;
  std::string current_adjust;
  std::string voltage_adjust;
  std::string unknown_18;
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
  std::string unknown_31;

  static std::optional<RegisterR7> from_fields(const std::vector<std::string> &fields) {
    if (fields.size() <= 30) {
      return std::nullopt;
    }

    RegisterR7 out;
    out.wcln_time = fields[0];
    out.ozone_off = fields[1];
    out.temperature_units = fields[2];
    out.ozone_24hrs = fields[3];
    out.cjet = fields[4];
    out.circulation_24hrs = fields[5];
    out.vele = fields[6];
    out.unknown_8 = fields[7];
    out.unknown_9 = fields[8];
    out.unknown_10 = fields[9];
    out.v_max = fields[10];
    out.v_min = fields[11];
    out.v_max_24hrs = fields[12];
    out.v_min_24hrs = fields[13];
    out.current_zero = fields[14];
    out.current_adjust = fields[15];
    out.voltage_adjust = fields[16];
    out.unknown_18 = fields[17];
    out.ser_1 = fields[18];
    out.ser_2 = fields[19];
    out.ser_3 = fields[20];
    out.vmax = fields[21];
    out.ahys = fields[22];
    out.huse = fields[23];
    out.hele = fields[24];
    out.hpmp = fields[25];
    out.pmin = fields[26];
    out.pflt = fields[27];
    out.phtr = fields[28];
    out.pmax = fields[29];
    out.unknown_31 = fields[30];
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

  std::string unknown_1;
  std::string unknown_2;
  std::string unknown_3;
  std::string unknown_4;
  std::string unknown_5;
  std::string unknown_6;
  std::string unknown_7;
  std::string unknown_8;
  std::string unknown_9;
  std::string outlet_blower;
  std::string unknown_11;
  std::string unknown_12;
  std::string unknown_13;
  std::string unknown_14;

  static std::optional<RegisterRC> from_fields(const std::vector<std::string> &fields) {
    if (fields.size() <= 13) {
      return std::nullopt;
    }

    RegisterRC out;
    out.unknown_1 = fields[0];
    out.unknown_2 = fields[1];
    out.unknown_3 = fields[2];
    out.unknown_4 = fields[3];
    out.unknown_5 = fields[4];
    out.unknown_6 = fields[5];
    out.unknown_7 = fields[6];
    out.unknown_8 = fields[7];
    out.unknown_9 = fields[8];
    out.outlet_blower = fields[9];
    out.unknown_11 = fields[10];
    out.unknown_12 = fields[11];
    out.unknown_13 = fields[12];
    out.unknown_14 = fields[13];
    return out;
  }
};

struct RegisterRE {
  static constexpr const char *kLabel = "RE";

  std::string hp_present;
  std::string unknown_2;
  std::string unknown_3;
  std::string unknown_4;
  std::string unknown_5;
  std::string unknown_6;
  std::string unknown_7;
  std::string unknown_8;
  std::string unknown_9;
  std::string hp_ambient;
  std::string hp_condensor;
  std::string hp_compressor_state;
  std::string hp_fan_state;
  std::string hp_4w_valve;
  std::string hp_heater_state;
  std::string hp_state;
  std::string hp_mode;
  std::string hp_defrost_timer;
  std::string hp_comp_run_timer;
  std::string hp_low_temp_timer;
  std::string hp_heat_accum_timer;
  std::string hp_sequence_timer;
  std::string hp_warning;
  std::string frez_tmr;
  std::string dbgn;
  std::string dend;
  std::string dcmp;
  std::string dmax;
  std::string dele;
  std::string dpmp;

  static std::optional<RegisterRE> from_fields(const std::vector<std::string> &fields) {
    if (fields.size() <= 29) {
      return std::nullopt;
    }

    RegisterRE out;
    out.hp_present = fields[0];
    out.unknown_2 = fields[1];
    out.unknown_3 = fields[2];
    out.unknown_4 = fields[3];
    out.unknown_5 = fields[4];
    out.unknown_6 = fields[5];
    out.unknown_7 = fields[6];
    out.unknown_8 = fields[7];
    out.unknown_9 = fields[8];
    out.hp_ambient = fields[9];
    out.hp_condensor = fields[10];
    out.hp_compressor_state = fields[11];
    out.hp_fan_state = fields[12];
    out.hp_4w_valve = fields[13];
    out.hp_heater_state = fields[14];
    out.hp_state = fields[15];
    out.hp_mode = fields[16];
    out.hp_defrost_timer = fields[17];
    out.hp_comp_run_timer = fields[18];
    out.hp_low_temp_timer = fields[19];
    out.hp_heat_accum_timer = fields[20];
    out.hp_sequence_timer = fields[21];
    out.hp_warning = fields[22];
    out.frez_tmr = fields[23];
    out.dbgn = fields[24];
    out.dend = fields[25];
    out.dcmp = fields[26];
    out.dmax = fields[27];
    out.dele = fields[28];
    out.dpmp = fields[29];
    return out;
  }
};

struct RegisterRG {
  static constexpr const char *kLabel = "RG";

  std::string pump1_ok_to_run;
  std::string pump2_ok_to_run;
  std::string pump3_ok_to_run;
  std::string pump4_ok_to_run;
  std::string pump5_ok_to_run;
  std::string unknown_6;
  std::string pump1_install_state;
  std::string pump2_install_state;
  std::string pump3_install_state;
  std::string pump4_install_state;
  std::string pump5_install_state;
  std::string lock_mode;
  std::string unknown_13;
  std::string unknown_14;

  static std::optional<RegisterRG> from_fields(const std::vector<std::string> &fields) {
    if (fields.size() < 14) {
      return std::nullopt;
    }

    RegisterRG out;
    out.pump1_ok_to_run = fields[0];
    out.pump2_ok_to_run = fields[1];
    out.pump3_ok_to_run = fields[2];
    out.pump4_ok_to_run = fields[3];
    out.pump5_ok_to_run = fields[4];
    out.unknown_6 = fields[5];
    out.pump1_install_state = fields[6];
    out.pump2_install_state = fields[7];
    out.pump3_install_state = fields[8];
    out.pump4_install_state = fields[9];
    out.pump5_install_state = fields[10];
    out.lock_mode = fields[11];
    out.unknown_13 = fields[12];
    out.unknown_14 = fields[13];
    return out;
  }
};

// Fallback for register labels that are not yet modelled with a dedicated
// typed struct.
struct UnknownRegisterLine {
  std::string label;
  std::vector<std::string> fields;
};

using AnyRegisterLine = std::variant<RegisterR2, RegisterR3, RegisterR4, RegisterR5, RegisterR6, RegisterR7, RegisterR9,
                                     RegisterRA, RegisterRB, RegisterRC, RegisterRE, RegisterRG, UnknownRegisterLine>;

// SpaNetParser is stateless. Every method is a pure function of its inputs.
class SpaNetParser {
 public:
  // Extract the register label from a line (e.g., "R2", "R3", "RG", etc.)
  // expects ,R2,...,...,... etc
  // Returns empty string if not a register line.
  static std::string extract_register_label(const std::string &line) {
    if (line.length() >= 4 && line[1] == 'R') {
      return line.substr(1, 2);
    }
    return "";
  }

  // Determines the routing of a single UART line (no embedded newlines).
  //   kStateUpdate -> feed into RegisterStore::update()
  //   kAck         -> match against a pending command expectation
  //   kUnknown     -> log and discard
  //
  // Standalone register continuation lines (",R3,...") are classified as kStateUpdate.
  static MessageType classify_message(const std::string &line) {
    auto trimmed = trim_(line);

    if (!extract_register_label(line).empty()) {
      return MessageType::kStateUpdate;
    }

    if (!trimmed.empty() && (trimmed[0] == 'S' || trimmed[0] == 'W')) {
      return MessageType::kAck;
    }

    return MessageType::kUnknown;
  }

  // Parses one kStateUpdate UART line into a typed AnyRegisterLine.
  // Returns nullopt if the line does not contain a recognisable register
  // pattern.
  //
  // Handles register lines in the format: ",R2:1,2,3"
  static std::optional<AnyRegisterLine> parse_register_line(const std::string &line) {
    auto content = trim_(line);

    if (!content.empty() && content[0] == ',') {
      content = content.substr(1);
    }

    // Should be "R?,field1,field2,etc"
    auto first_comma = content.find(',');
    if (first_comma == std::string::npos) {
      return std::nullopt;
    }
    auto label = content.substr(0, first_comma);
    if (!is_register_label_(label)) {
      return std::nullopt;
    }

    auto fields = split_by_comma_(content.substr(first_comma + 1));
    return decode_register_line_(label, std::move(fields));
  }

 private:
  static AnyRegisterLine decode_register_line_(const std::string &label, const std::vector<std::string> &fields) {
    if (label == "R2") {
      if (auto reg = RegisterR2::from_fields(fields)) {
        return reg.value();
      }
    } else if (label == "R3") {
      if (auto reg = RegisterR3::from_fields(fields)) {
        return reg.value();
      }
    } else if (label == "R4") {
      if (auto reg = RegisterR4::from_fields(fields)) {
        return reg.value();
      }
    } else if (label == "R5") {
      if (auto reg = RegisterR5::from_fields(fields)) {
        return reg.value();
      }
    } else if (label == "R6") {
      if (auto reg = RegisterR6::from_fields(fields)) {
        return reg.value();
      }
    } else if (label == "R7") {
      if (auto reg = RegisterR7::from_fields(fields)) {
        return reg.value();
      }
    } else if (label == "R9") {
      if (auto reg = RegisterR9::from_fields(fields)) {
        return reg.value();
      }
    } else if (label == "RA") {
      if (auto reg = RegisterRA::from_fields(fields)) {
        return reg.value();
      }
    } else if (label == "RB") {
      if (auto reg = RegisterRB::from_fields(fields)) {
        return reg.value();
      }
    } else if (label == "RC") {
      if (auto reg = RegisterRC::from_fields(fields)) {
        return reg.value();
      }
    } else if (label == "RE") {
      if (auto reg = RegisterRE::from_fields(fields)) {
        return reg.value();
      }
    } else if (label == "RG") {
      if (auto reg = RegisterRG::from_fields(fields)) {
        return reg.value();
      }
    }
    return UnknownRegisterLine{label, fields};
  }

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

}  // namespace esphome::spanet
