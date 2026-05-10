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

// ──────────────────────────────────────────────────────────────
// Architecture and API contract
// ──────────────────────────────────────────────────────────────
//
// Three layers between raw UART bytes and typed state:
//
//   UartRxBuffer       Byte framing. Emits one line per \n, stripped
//                      of the trailing newline. No protocol knowledge.
//
//   SpaNetParser       Stateless. Classifies a single line and parses
//                      one register line into its label and field list.
//                      Holds no state between calls.
//
//   RfRegisterStore    Stateful accumulator. Holds the latest known
//                      fields for every register label seen so far.
//                      Updated one line at a time. Exposes typed
//                      accessors that read from its accumulated data.
//
// A complete SpaNET RF response spans multiple UART lines — one per
// register (R2, R3 … RG). The store handles this naturally: each line
// updates only the entry for its own label. Registers not yet received
// remain absent from the store and do not block access to registers
// that have already arrived.
//
// ──────────────────────────────────────────────────────────────
// Caller responsibilities
// ──────────────────────────────────────────────────────────────
//
//   1. Call classify_message on each line from UartRxBuffer.
//
//   2. Feed kStateUpdate lines into RfRegisterStore::update().
//      No multi-line assembly step is needed.
//
//   3. Route kAck lines to command-ack handling, not to the store.
//
//   4. Read typed state from the store when needed (e.g. in update()).
//      The store always reflects the most recently seen value for each
//      register, regardless of whether the full RF response is complete.

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

  // All offsets below are mapped from ESPySpa's updateMeasures() R4 logic.
  // fields[0] corresponds to R4+1 in ESPySpa, fields[24] to R4+25.
  std::vector<std::string> fields;
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
    // Need fields through R4+25 (0-based index 24 in this vector).
    if (fields.size() <= 24) {
      return std::nullopt;
    }

    RegisterR4 out;
    out.fields = fields;
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

// Fallback for register labels that are not yet modelled with a dedicated
// typed struct.
struct UnknownRegisterLine {
  std::string label;
  std::vector<std::string> fields;
};

using AnyRegisterLine = std::variant<RegisterR2, RegisterR3, RegisterR4, UnknownRegisterLine>;

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

    // Strip "RF:" prefix and optional leading comma from the first response line.
    if (content.rfind("RF:", 0) == 0) {
      content = content.substr(3);
      if (!content.empty() && content[0] == ',') {
        content = content.substr(1);
      }
    }

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
