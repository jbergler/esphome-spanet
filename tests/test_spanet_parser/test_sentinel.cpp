#include <cstdio>
#include <string>

#include <gtest/gtest.h>

#include "../../components/spanet/spanet_parser.h"

namespace esphome::spanet::tests {

static std::string field_token(int one_based_index) {
  char buffer[8];
  std::snprintf(buffer, sizeof(buffer), "f%02d", one_based_index);
  return std::string(buffer);
}

static std::string build_register_line(const std::string &label, int field_count) {
  std::string line = "," + label;
  for (int index = 1; index <= field_count; index++) {
    line += ",";
    line += field_token(index);
  }
  line += ",:";
  return line;
}

TEST(ParseRegisterLineSentinelTest, R2Mapping) {
  auto result = SpaNetParser::parse_register_line(build_register_line("R2", 29));
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterR2>(result.value()));

  const auto &r2 = std::get<RegisterR2>(result.value());
  EXPECT_EQ(r2.mains_current, field_token(1));
  EXPECT_EQ(r2.mains_voltage, field_token(2));
  EXPECT_EQ(r2.case_temperature, field_token(3));
  EXPECT_EQ(r2.port_current, field_token(4));
  EXPECT_EQ(r2.spa_day_of_week, field_token(5));
  EXPECT_EQ(r2.spa_time_hour, field_token(6));
  EXPECT_EQ(r2.spa_time_minute, field_token(7));
  EXPECT_EQ(r2.spa_time_second, field_token(8));
  EXPECT_EQ(r2.spa_time_day, field_token(9));
  EXPECT_EQ(r2.spa_time_month, field_token(10));
  EXPECT_EQ(r2.spa_time_year, field_token(11));
  EXPECT_EQ(r2.heater_temperature, field_token(12));
  EXPECT_EQ(r2.pool_temperature, field_token(13));
  EXPECT_EQ(r2.water_present, field_token(14));
  EXPECT_EQ(r2.unknown_15, field_token(15));
  EXPECT_EQ(r2.awake_minutes_remaining, field_token(16));
  EXPECT_EQ(r2.filt_pump_run_time_total, field_token(17));
  EXPECT_EQ(r2.filt_pump_req_mins, field_token(18));
  EXPECT_EQ(r2.load_timeout, field_token(19));
  EXPECT_EQ(r2.hour_meter, field_token(20));
  EXPECT_EQ(r2.relay_1, field_token(21));
  EXPECT_EQ(r2.relay_2, field_token(22));
  EXPECT_EQ(r2.relay_3, field_token(23));
  EXPECT_EQ(r2.relay_4, field_token(24));
  EXPECT_EQ(r2.relay_5, field_token(25));
  EXPECT_EQ(r2.relay_6, field_token(26));
  EXPECT_EQ(r2.relay_7, field_token(27));
  EXPECT_EQ(r2.relay_8, field_token(28));
  EXPECT_EQ(r2.relay_9, field_token(29));
}

TEST(ParseRegisterLineSentinelTest, R3Mapping) {
  auto result = SpaNetParser::parse_register_line(build_register_line("R3", 9));
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterR3>(result.value()));

  const auto &r3 = std::get<RegisterR3>(result.value());
  EXPECT_EQ(r3.software_version, field_token(6));
  EXPECT_EQ(r3.model, field_token(7));
  EXPECT_EQ(r3.serial_number_1, field_token(8));
  EXPECT_EQ(r3.serial_number_2, field_token(9));
}

TEST(ParseRegisterLineSentinelTest, R4Mapping) {
  auto result = SpaNetParser::parse_register_line(build_register_line("R4", 25));
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterR4>(result.value()));

  const auto &r4 = std::get<RegisterR4>(result.value());
  EXPECT_EQ(r4.mode, field_token(1));
  EXPECT_EQ(r4.ser1_timer, field_token(2));
  EXPECT_EQ(r4.ser2_timer, field_token(3));
  EXPECT_EQ(r4.ser3_timer, field_token(4));
  EXPECT_EQ(r4.heat_mode, field_token(5));
  EXPECT_EQ(r4.pump_idle_timer, field_token(6));
  EXPECT_EQ(r4.pump_run_timer, field_token(7));
  EXPECT_EQ(r4.adt_pool_hys, field_token(8));
  EXPECT_EQ(r4.adt_heater_hys, field_token(9));
  EXPECT_EQ(r4.power, field_token(10));
  EXPECT_EQ(r4.power_kwh, field_token(11));
  EXPECT_EQ(r4.power_today, field_token(12));
  EXPECT_EQ(r4.power_yesterday, field_token(13));
  EXPECT_EQ(r4.thermal_cut_out, field_token(14));
  EXPECT_EQ(r4.test_d1, field_token(15));
  EXPECT_EQ(r4.test_d2, field_token(16));
  EXPECT_EQ(r4.test_d3, field_token(17));
  EXPECT_EQ(r4.element_heat_source_offset, field_token(18));
  EXPECT_EQ(r4.frequency, field_token(19));
  EXPECT_EQ(r4.hp_heat_source_offset_heat, field_token(20));
  EXPECT_EQ(r4.hp_heat_source_offset_cool, field_token(21));
  EXPECT_EQ(r4.heat_source_off_time, field_token(22));
  EXPECT_EQ(r4.vari_mode, field_token(23));
  EXPECT_EQ(r4.vari_speed, field_token(24));
  EXPECT_EQ(r4.vari_percent, field_token(25));
}

TEST(ParseRegisterLineSentinelTest, R5Mapping) {
  auto result = SpaNetParser::parse_register_line(build_register_line("R5", 26));
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterR5>(result.value()));

  const auto &r5 = std::get<RegisterR5>(result.value());
  EXPECT_EQ(r5.status_0, field_token(1));
  EXPECT_EQ(r5.status_1, field_token(2));
  EXPECT_EQ(r5.status_2, field_token(3));
  EXPECT_EQ(r5.status_3, field_token(4));
  EXPECT_EQ(r5.status_4, field_token(5));
  EXPECT_EQ(r5.status_5, field_token(6));
  EXPECT_EQ(r5.status_6, field_token(7));
  EXPECT_EQ(r5.status_7, field_token(8));
  EXPECT_EQ(r5.status_8, field_token(9));
  EXPECT_EQ(r5.status_9, field_token(10));
  EXPECT_EQ(r5.status_10, field_token(11));
  EXPECT_EQ(r5.status_11, field_token(12));
  EXPECT_EQ(r5.status_12, field_token(13));
  EXPECT_EQ(r5.status_13, field_token(14));
  EXPECT_EQ(r5.status_14, field_token(15));
  EXPECT_EQ(r5.status_15, field_token(16));
  EXPECT_EQ(r5.status_16, field_token(17));
  EXPECT_EQ(r5.status_17, field_token(18));
  EXPECT_EQ(r5.status_18, field_token(19));
  EXPECT_EQ(r5.status_19, field_token(20));
  EXPECT_EQ(r5.status_20, field_token(21));
  EXPECT_EQ(r5.status_21, field_token(22));
  EXPECT_EQ(r5.status_22, field_token(23));
  EXPECT_EQ(r5.status_23, field_token(24));
  EXPECT_EQ(r5.status_24, field_token(25));
  EXPECT_EQ(r5.status_25, field_token(26));
}

TEST(ParseRegisterLineSentinelTest, R6Mapping) {
  auto result = SpaNetParser::parse_register_line(build_register_line("R6", 28));
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterR6>(result.value()));

  const auto &r6 = std::get<RegisterR6>(result.value());
  EXPECT_EQ(r6.vari_value, field_token(1));
  EXPECT_EQ(r6.brightness, field_token(2));
  EXPECT_EQ(r6.current_color, field_token(3));
  EXPECT_EQ(r6.color_mode, field_token(4));
  EXPECT_EQ(r6.light_effect_speed, field_token(5));
  EXPECT_EQ(r6.filt_set_hrs, field_token(6));
  EXPECT_EQ(r6.filt_block_hrs, field_token(7));
  EXPECT_EQ(r6.set_temperature, field_token(8));
  EXPECT_EQ(r6.l_24hours, field_token(9));
  EXPECT_EQ(r6.power_save_level, field_token(10));
  EXPECT_EQ(r6.peak_power_begin, field_token(11));
  EXPECT_EQ(r6.peak_power_end, field_token(12));
  EXPECT_EQ(r6.sleep_timer_1_day, field_token(13));
  EXPECT_EQ(r6.sleep_timer_2_day, field_token(14));
  EXPECT_EQ(r6.sleep_timer_1_begin, field_token(15));
  EXPECT_EQ(r6.sleep_timer_2_begin, field_token(16));
  EXPECT_EQ(r6.sleep_timer_1_end, field_token(17));
  EXPECT_EQ(r6.sleep_timer_2_end, field_token(18));
  EXPECT_EQ(r6.default_screen, field_token(19));
  EXPECT_EQ(r6.timeout, field_token(20));
  EXPECT_EQ(r6.variable_pump, field_token(21));
  EXPECT_EQ(r6.hifi, field_token(22));
  EXPECT_EQ(r6.brand, field_token(23));
  EXPECT_EQ(r6.prime, field_token(24));
  EXPECT_EQ(r6.element, field_token(25));
  EXPECT_EQ(r6.type, field_token(26));
  EXPECT_EQ(r6.gas, field_token(27));
  EXPECT_EQ(r6.unknown_28, field_token(28));
}

TEST(ParseRegisterLineSentinelTest, R7Mapping) {
  auto result = SpaNetParser::parse_register_line(build_register_line("R7", 31));
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterR7>(result.value()));

  const auto &r7 = std::get<RegisterR7>(result.value());
  EXPECT_EQ(r7.wcln_time, field_token(1));
  EXPECT_EQ(r7.ozone_off, field_token(2));
  EXPECT_EQ(r7.temperature_units, field_token(3));
  EXPECT_EQ(r7.ozone_24hrs, field_token(4));
  EXPECT_EQ(r7.cjet, field_token(5));
  EXPECT_EQ(r7.circulation_24hrs, field_token(6));
  EXPECT_EQ(r7.vele, field_token(7));
  EXPECT_EQ(r7.unknown_8, field_token(8));
  EXPECT_EQ(r7.unknown_9, field_token(9));
  EXPECT_EQ(r7.unknown_10, field_token(10));
  EXPECT_EQ(r7.v_max, field_token(11));
  EXPECT_EQ(r7.v_min, field_token(12));
  EXPECT_EQ(r7.v_max_24hrs, field_token(13));
  EXPECT_EQ(r7.v_min_24hrs, field_token(14));
  EXPECT_EQ(r7.current_zero, field_token(15));
  EXPECT_EQ(r7.current_adjust, field_token(16));
  EXPECT_EQ(r7.voltage_adjust, field_token(17));
  EXPECT_EQ(r7.unknown_18, field_token(18));
  EXPECT_EQ(r7.ser_1, field_token(19));
  EXPECT_EQ(r7.ser_2, field_token(20));
  EXPECT_EQ(r7.ser_3, field_token(21));
  EXPECT_EQ(r7.vmax, field_token(22));
  EXPECT_EQ(r7.ahys, field_token(23));
  EXPECT_EQ(r7.huse, field_token(24));
  EXPECT_EQ(r7.hele, field_token(25));
  EXPECT_EQ(r7.hpmp, field_token(26));
  EXPECT_EQ(r7.pmin, field_token(27));
  EXPECT_EQ(r7.pflt, field_token(28));
  EXPECT_EQ(r7.phtr, field_token(29));
  EXPECT_EQ(r7.pmax, field_token(30));
  EXPECT_EQ(r7.unknown_31, field_token(31));
}

TEST(ParseRegisterLineSentinelTest, R9Mapping) {
  auto result = SpaNetParser::parse_register_line(build_register_line("R9", 12));
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterR9>(result.value()));

  const auto &r9 = std::get<RegisterR9>(result.value());
  EXPECT_EQ(r9.fault_code, field_token(1));
  EXPECT_EQ(r9.accum_1, field_token(2));
  EXPECT_EQ(r9.accum_2, field_token(3));
  EXPECT_EQ(r9.accum_3, field_token(4));
  EXPECT_EQ(r9.accum_4, field_token(5));
  EXPECT_EQ(r9.accum_5, field_token(6));
  EXPECT_EQ(r9.accum_6, field_token(7));
  EXPECT_EQ(r9.accum_7, field_token(8));
  EXPECT_EQ(r9.accum_8, field_token(9));
  EXPECT_EQ(r9.accum_9, field_token(10));
  EXPECT_EQ(r9.accum_10, field_token(11));
  EXPECT_EQ(r9.accum_11, field_token(12));
}

TEST(ParseRegisterLineSentinelTest, RAMapping) {
  auto result = SpaNetParser::parse_register_line(build_register_line("RA", 12));
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterRA>(result.value()));

  const auto &ra = std::get<RegisterRA>(result.value());
  EXPECT_EQ(ra.fault_code, field_token(1));
  EXPECT_EQ(ra.accum_1, field_token(2));
  EXPECT_EQ(ra.accum_2, field_token(3));
  EXPECT_EQ(ra.accum_3, field_token(4));
  EXPECT_EQ(ra.accum_4, field_token(5));
  EXPECT_EQ(ra.accum_5, field_token(6));
  EXPECT_EQ(ra.accum_6, field_token(7));
  EXPECT_EQ(ra.accum_7, field_token(8));
  EXPECT_EQ(ra.accum_8, field_token(9));
  EXPECT_EQ(ra.accum_9, field_token(10));
  EXPECT_EQ(ra.accum_10, field_token(11));
  EXPECT_EQ(ra.accum_11, field_token(12));
}

TEST(ParseRegisterLineSentinelTest, RBMapping) {
  auto result = SpaNetParser::parse_register_line(build_register_line("RB", 12));
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterRB>(result.value()));

  const auto &rb = std::get<RegisterRB>(result.value());
  EXPECT_EQ(rb.fault_code, field_token(1));
  EXPECT_EQ(rb.accum_1, field_token(2));
  EXPECT_EQ(rb.accum_2, field_token(3));
  EXPECT_EQ(rb.accum_3, field_token(4));
  EXPECT_EQ(rb.accum_4, field_token(5));
  EXPECT_EQ(rb.accum_5, field_token(6));
  EXPECT_EQ(rb.accum_6, field_token(7));
  EXPECT_EQ(rb.accum_7, field_token(8));
  EXPECT_EQ(rb.accum_8, field_token(9));
  EXPECT_EQ(rb.accum_9, field_token(10));
  EXPECT_EQ(rb.accum_10, field_token(11));
  EXPECT_EQ(rb.accum_11, field_token(12));
}

TEST(ParseRegisterLineSentinelTest, RCMapping) {
  auto result = SpaNetParser::parse_register_line(build_register_line("RC", 14));
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterRC>(result.value()));

  const auto &rc = std::get<RegisterRC>(result.value());
  EXPECT_EQ(rc.unknown_1, field_token(1));
  EXPECT_EQ(rc.unknown_2, field_token(2));
  EXPECT_EQ(rc.unknown_3, field_token(3));
  EXPECT_EQ(rc.unknown_4, field_token(4));
  EXPECT_EQ(rc.unknown_5, field_token(5));
  EXPECT_EQ(rc.unknown_6, field_token(6));
  EXPECT_EQ(rc.unknown_7, field_token(7));
  EXPECT_EQ(rc.unknown_8, field_token(8));
  EXPECT_EQ(rc.unknown_9, field_token(9));
  EXPECT_EQ(rc.outlet_blower, field_token(10));
  EXPECT_EQ(rc.unknown_11, field_token(11));
  EXPECT_EQ(rc.unknown_12, field_token(12));
  EXPECT_EQ(rc.unknown_13, field_token(13));
  EXPECT_EQ(rc.unknown_14, field_token(14));
}

TEST(ParseRegisterLineSentinelTest, REMapping) {
  auto result = SpaNetParser::parse_register_line(build_register_line("RE", 30));
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterRE>(result.value()));

  const auto &re = std::get<RegisterRE>(result.value());
  EXPECT_EQ(re.hp_present, field_token(1));
  EXPECT_EQ(re.unknown_2, field_token(2));
  EXPECT_EQ(re.unknown_3, field_token(3));
  EXPECT_EQ(re.unknown_4, field_token(4));
  EXPECT_EQ(re.unknown_5, field_token(5));
  EXPECT_EQ(re.unknown_6, field_token(6));
  EXPECT_EQ(re.unknown_7, field_token(7));
  EXPECT_EQ(re.unknown_8, field_token(8));
  EXPECT_EQ(re.unknown_9, field_token(9));
  EXPECT_EQ(re.hp_ambient, field_token(10));
  EXPECT_EQ(re.hp_condensor, field_token(11));
  EXPECT_EQ(re.hp_compressor_state, field_token(12));
  EXPECT_EQ(re.hp_fan_state, field_token(13));
  EXPECT_EQ(re.hp_4w_valve, field_token(14));
  EXPECT_EQ(re.hp_heater_state, field_token(15));
  EXPECT_EQ(re.hp_state, field_token(16));
  EXPECT_EQ(re.hp_mode, field_token(17));
  EXPECT_EQ(re.hp_defrost_timer, field_token(18));
  EXPECT_EQ(re.hp_comp_run_timer, field_token(19));
  EXPECT_EQ(re.hp_low_temp_timer, field_token(20));
  EXPECT_EQ(re.hp_heat_accum_timer, field_token(21));
  EXPECT_EQ(re.hp_sequence_timer, field_token(22));
  EXPECT_EQ(re.hp_warning, field_token(23));
  EXPECT_EQ(re.frez_tmr, field_token(24));
  EXPECT_EQ(re.dbgn, field_token(25));
  EXPECT_EQ(re.dend, field_token(26));
  EXPECT_EQ(re.dcmp, field_token(27));
  EXPECT_EQ(re.dmax, field_token(28));
  EXPECT_EQ(re.dele, field_token(29));
  EXPECT_EQ(re.dpmp, field_token(30));
}

TEST(ParseRegisterLineSentinelTest, RGMapping) {
  auto result = SpaNetParser::parse_register_line(build_register_line("RG", 14));
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterRG>(result.value()));

  const auto &rg = std::get<RegisterRG>(result.value());
  EXPECT_EQ(rg.pump1_ok_to_run, field_token(1));
  EXPECT_EQ(rg.pump2_ok_to_run, field_token(2));
  EXPECT_EQ(rg.pump3_ok_to_run, field_token(3));
  EXPECT_EQ(rg.pump4_ok_to_run, field_token(4));
  EXPECT_EQ(rg.pump5_ok_to_run, field_token(5));
  EXPECT_EQ(rg.unknown_6, field_token(6));
  EXPECT_EQ(rg.pump1_install_state, field_token(7));
  EXPECT_EQ(rg.pump2_install_state, field_token(8));
  EXPECT_EQ(rg.pump3_install_state, field_token(9));
  EXPECT_EQ(rg.pump4_install_state, field_token(10));
  EXPECT_EQ(rg.pump5_install_state, field_token(11));
  EXPECT_EQ(rg.lock_mode, field_token(12));
  EXPECT_EQ(rg.unknown_13, field_token(13));
  EXPECT_EQ(rg.unknown_14, field_token(14));
}

}  // namespace esphome::spanet::tests
