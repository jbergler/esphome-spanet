#include <fstream>
#include <string>

#include <gtest/gtest.h>

#include "../../components/spanet/spanet_state.h"
#include "../../components/spanet/uart_rx_buffer.h"

namespace esphome::spanet::tests {

static bool put_line(RegisterStore &store, const std::string &line) { return store.update(line); }

TEST(RegisterStoreTest, ReturnsEmptyControllerStatusBeforeR3IsReceived) {
  RegisterStore store;
  put_line(store, "R2:1,2,3");

  const auto &state = store.get_state();
  EXPECT_TRUE(state.controller_status.model.empty());
}

TEST(RegisterStoreTest, ExtractsIdentityAfterR3Line) {
  RegisterStore store;
  put_line(store, ",R3,10,20,30,40,50,SW V3.1,SVM1,SN123,SN456,:");

  const auto &state = store.get_state();
  EXPECT_EQ(state.controller_status.software_version, "V3.1");
  EXPECT_EQ(state.controller_status.model, "SVM1");
  EXPECT_EQ(state.controller_status.serial_number, "SN123-SN456");
}

TEST(RegisterStoreTest, PartialUpdateOnlyChangesOneRegister) {
  RegisterStore store;
  put_line(store, ",R3,10,20,30,40,50,SW V3.1,SVM1,SN123,SN456,:");
  put_line(store, ",R2,1,2,3,:");

  const auto &state = store.get_state();
  EXPECT_EQ(state.controller_status.model, "SVM1");
}

TEST(RegisterStoreTest, OverwritesRegisterOnDuplicateLine) {
  RegisterStore store;
  put_line(store, ",R3,10,20,30,40,50,SW V3.1,OLD_MODEL,SN123,SN456,:");
  put_line(store, ",R3,10,20,30,40,50,SW V3.2,NEW_MODEL,SN123,SN456,:");

  const auto &state = store.get_state();
  EXPECT_EQ(state.controller_status.model, "NEW_MODEL");
  EXPECT_EQ(state.controller_status.software_version, "V3.2");
}

TEST(RegisterStoreTest, ReturnsEmptyWhenR3HasTooFewFields) {
  RegisterStore store;
  put_line(store, ",R3,10,20,:");

  const auto &state = store.get_state();
  EXPECT_TRUE(state.controller_status.model.empty());
}

TEST(RegisterStoreTest, UpdateReturnsFalseForNonRegisterLine) {
  RegisterStore store;
  EXPECT_FALSE(put_line(store, "HELLO"));
  EXPECT_FALSE(put_line(store, "S22-OK"));
}

TEST(RegisterStoreTest, UpdateReturnsTrueForUnknownRegisterLabel) {
  RegisterStore store;
  EXPECT_TRUE(put_line(store, ",RZ,1,2,3,:"));
}

TEST(RegisterStoreTest, UpdateReturnsTrueForValidLine) {
  RegisterStore store;
  EXPECT_TRUE(put_line(store, ",R3,10,20,30,40,50,SW V3.1,SVM1,SN123,SN456,:"));
  EXPECT_TRUE(put_line(store, "RF:,R2,0,239,40,81,0,10,46,36,11,5,2026,385,9999,1,0,674,127,0,"
                              "6000,342132,42286,40243,44,0,0,0,650,39660,42484,126,:"));
}

TEST(RegisterStoreTest, StoresTypedRegistersInRegistersStruct) {
  RegisterStore store;
  put_line(store, ",R3,10,20,30,40,50,SW V3.1,SVM1,SN123,SN456,:");
  put_line(store, ",R4,NORM,0,0,0,2,0,254,4,20,0,0,0,0,0,0,0,262144,3,0,101,0,"
                  "2022,6,80,50,0,0,5,:");

  const auto &regs = store.get_registers();
  ASSERT_TRUE(regs.r3.has_value());
  ASSERT_TRUE(regs.r4.has_value());
  EXPECT_EQ(regs.r3.value().model, "SVM1");
  EXPECT_EQ(regs.r4.value().mode, "NORM");
}

TEST(RegisterStoreTest, HandlesCompleteRealRfPayload) {
  RegisterStore store;

  put_line(store, "RF:,R2,0,239,40,81,0,10,46,36,11,5,2026,385,9999,1,0,674,127,0,"
                  "6000,342132,42286,40243,44,0,0,0,650,39660,42484,126,:");
  put_line(store, ",R3,10,1,4,4,4,SW V6 21 12 "
                  "13,SVM1,21460001,20000999,0,1,0,0,0,0,NA,1,0,414,Auto,650,0,"
                  "7,7,0,0,0,:");
  put_line(store, ",R4,NORM,0,0,0,2,0,254,4,20,0,0,0,0,0,0,0,262144,3,0,101,0,"
                  "2022,6,80,50,0,0,5,:");
  put_line(store, ",R5,0,1,0,5,0,0,0,0,0,0,1,0,1,0,394,0,23,0,4,0,0,0,1,2,6,6,:");
  put_line(store, ",R6,5,3,1,1,5,1,4,390,1,0,3584,5120,31,96,5632,5918,1792,"
                  "1792,0,30,0,0,0,0,1,5,0,410,:");
  put_line(store, ",R7,1792,0,1,0,1,0,0,6,2,2023,250,217,238,226,280,125,136,1,"
                  "0,0,0,23,200,1,0,1,31,50,50,100,5,:");
  put_line(store, ",R9,F1,1023,5667,5,0,236,9999,455,46,0,255,27516,:");
  put_line(store, ",RA,F2,880,5679,4,0,231,9999,496,47,0,200,380,:");
  put_line(store, ",RB,F3,879,5890,4,0,242,9999,208,46,0,255,380,:");
  put_line(store, ",RC,0,1,1,0,0,0,0,0,0,2,0,0,0,0,:");
  put_line(store, ",RE,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,-4,13,30,8,5,1,:");
  put_line(store, ",RG,1,1,1,1,1,1,0-,1-2-0324,1-1-01,0-,0-,0,0,0,1808,:");

  const auto &state = store.get_state();
  EXPECT_EQ(state.controller_status.software_version, "V6.21.12.13");
  EXPECT_EQ(state.controller_status.model, "SVM1");
  EXPECT_EQ(state.controller_status.serial_number, "21460001-20000999");

  const auto &regs = store.get_registers();
  ASSERT_TRUE(regs.r2.has_value());
  ASSERT_TRUE(regs.r3.has_value());
  ASSERT_TRUE(regs.r4.has_value());
  ASSERT_TRUE(regs.r5.has_value());
  ASSERT_TRUE(regs.r6.has_value());
  ASSERT_TRUE(regs.r7.has_value());
  ASSERT_TRUE(regs.r9.has_value());
  ASSERT_TRUE(regs.ra.has_value());
  ASSERT_TRUE(regs.rb.has_value());
  ASSERT_TRUE(regs.rc.has_value());
  ASSERT_TRUE(regs.re.has_value());
  ASSERT_TRUE(regs.rg.has_value());
}

TEST(RegisterStoreTest, ParsesR2DatetimeIntoCurrentTime) {
  RegisterStore store;
  put_line(store, ",R2,0,239,40,81,0,10,46,36,11,5,2026,385,9999,1,0,674,127,0,"
                  "6000,342132,42286,40243,44,0,0,0,650,39660,42484,126,:");

  const auto &state = store.get_state();
  ASSERT_TRUE(state.controller_status.current_time.has_value());
  EXPECT_GT(state.controller_status.current_time.value(), 946684800);
}

TEST(RegisterStoreTest, ParsesWaterAndSetpointTemperaturesFromR5AndR6) {
  RegisterStore store;
  put_line(store, ",R5,0,1,0,5,0,0,0,0,0,0,1,0,1,0,394,0,23,0,4,0,0,0,1,2,6,6,:");
  put_line(store, ",R6,5,3,1,1,5,1,4,390,1,0,3584,5120,31,96,5632,5918,1792,"
                  "1792,0,30,0,0,0,0,1,5,0,410,:");

  const auto &state = store.get_state();
  ASSERT_TRUE(state.temperatures.water_c.has_value());
  ASSERT_TRUE(state.temperatures.setpoint_c.has_value());
  EXPECT_NEAR(state.temperatures.water_c.value(), 39.4f, 0.01f);
  EXPECT_NEAR(state.temperatures.setpoint_c.value(), 39.0f, 0.01f);
}

TEST(RegisterStoreTest, ParsesHeatingActiveFromR5HeaterReadback) {
  RegisterStore store;
  put_line(store, ",R5,0,1,0,5,0,0,0,0,0,0,1,1,1,0,394,0,23,0,4,0,0,0,1,2,6,6,:");

  const auto &state = store.get_state();
  ASSERT_TRUE(state.climate.heating_active.has_value());
  EXPECT_TRUE(state.climate.heating_active.value());
}

TEST(RegisterStoreTest, ParsesHeatingInactiveFromR5HeaterReadback) {
  RegisterStore store;
  put_line(store, ",R5,0,1,0,5,0,0,0,0,0,0,0,0,1,0,394,0,23,0,4,0,0,0,1,2,6,6,:");

  const auto &state = store.get_state();
  ASSERT_TRUE(state.climate.heating_active.has_value());
  EXPECT_FALSE(state.climate.heating_active.value());
}

TEST(RegisterStoreTest, ParsesR2TelemetryWithExpectedScaling) {
  RegisterStore store;
  put_line(store, ",R2,77,240,245,81,0,10,46,36,11,5,2026,385,9999,1,0,674,127,"
                  "0,6000,342132,42286,40243,44,0,0,0,650,39660,42484,126,:");

  const auto &state = store.get_state();
  ASSERT_TRUE(state.power.mains_current_a.has_value());
  ASSERT_TRUE(state.power.mains_voltage_v.has_value());
  ASSERT_TRUE(state.temperatures.case_c.has_value());
  ASSERT_TRUE(state.temperatures.heater_c.has_value());
  EXPECT_NEAR(state.power.mains_current_a.value(), 7.7f, 0.01f);
  EXPECT_NEAR(state.power.mains_voltage_v.value(), 240.0f, 0.01f);
  EXPECT_NEAR(state.temperatures.case_c.value(), 24.5f, 0.01f);
  EXPECT_NEAR(state.temperatures.heater_c.value(), 38.5f, 0.01f);
}

TEST(RegisterStoreTest, ParsesR4PowerTelemetryWithExpectedScaling) {
  RegisterStore store;
  put_line(store, ",R4,NORM,0,0,0,2,0,254,4,20,24350,12345,0,0,0,0,0,0,262144,"
                  "3,0,101,0,2022,6,80,50,0,0,5,:");

  const auto &state = store.get_state();
  ASSERT_TRUE(state.power.instant_power_w.has_value());
  ASSERT_TRUE(state.power.total_energy_kwh.has_value());
  EXPECT_NEAR(state.power.instant_power_w.value(), 2435.0f, 0.01f);
  EXPECT_NEAR(state.power.total_energy_kwh.value(), 123.45f, 0.01f);
}

TEST(RegisterStoreTest, ParsesPumpCapabilitiesAndRuntimeStateFromRgAndR5) {
  RegisterStore store;
  put_line(store, ",R5,0,1,0,5,0,0,0,0,0,0,0,0,1,0,392,0,0,0,2,1,0,0,1,2,6,6,:");
  put_line(store, ",RG,1,1,1,1,1,1,0-,1-2-0324,1-1-01,0-,0-,0,0,0,1808,:");

  const auto &state = store.get_state();

  EXPECT_FALSE(state.pumps[0].installed);
  EXPECT_FALSE(state.pumps[0].capabilities_valid);

  ASSERT_TRUE(state.pumps[1].installed);
  ASSERT_TRUE(state.pumps[1].capabilities_valid);
  EXPECT_EQ(state.pumps[1].speed_type, 2);
  EXPECT_TRUE(state.pumps[1].supports_raw_mode[0]);
  EXPECT_TRUE(state.pumps[1].supports_raw_mode[2]);
  EXPECT_TRUE(state.pumps[1].supports_raw_mode[3]);
  EXPECT_TRUE(state.pumps[1].supports_raw_mode[4]);
  EXPECT_TRUE(state.pumps[1].supports_speed);
  EXPECT_TRUE(state.pumps[1].supports_auto);
  EXPECT_EQ(state.pumps[1].manual_raw_mode_count, 2u);
  EXPECT_EQ(state.pumps[1].manual_raw_modes[0], 2);
  EXPECT_EQ(state.pumps[1].manual_raw_modes[1], 3);
  ASSERT_TRUE(state.pumps[1].current_raw_mode.has_value());
  EXPECT_EQ(state.pumps[1].current_raw_mode.value(), 2);
  EXPECT_TRUE(state.pumps[1].is_on);
  EXPECT_FALSE(state.pumps[1].auto_mode_active);

  ASSERT_TRUE(state.pumps[2].installed);
  ASSERT_TRUE(state.pumps[2].capabilities_valid);
  EXPECT_EQ(state.pumps[2].speed_type, 1);
  EXPECT_TRUE(state.pumps[2].supports_raw_mode[0]);
  EXPECT_TRUE(state.pumps[2].supports_raw_mode[1]);
  EXPECT_EQ(state.pumps[2].manual_raw_mode_count, 1u);
  EXPECT_FALSE(state.pumps[2].supports_speed);
  ASSERT_TRUE(state.pumps[2].current_raw_mode.has_value());
  EXPECT_EQ(state.pumps[2].current_raw_mode.value(), 1);
  EXPECT_TRUE(state.pumps[2].is_on);
}

TEST(RegisterStoreTest, TreatsMalformedPumpInstallStateAsUnavailable) {
  RegisterStore store;
  put_line(store, ",RG,1,1,1,1,1,1,BAD,1-2-0324,1-1-01,0-,0-,0,0,0,1808,:");

  const auto &state = store.get_state();
  EXPECT_FALSE(state.pumps[0].installed);
  EXPECT_FALSE(state.pumps[0].capabilities_valid);

  EXPECT_TRUE(state.pumps[1].installed);
  EXPECT_TRUE(state.pumps[1].capabilities_valid);
}

TEST(ControllerStatusIntegrationTest, ParsesFullExamplePayloadFromFile) {
  std::ifstream file("tests/data/example-1.txt");
  ASSERT_TRUE(file.is_open()) << "Could not open tests/data/example-1.txt";

  UartRxBuffer rx{256};
  RegisterStore store;

  char ch;
  while (file.get(ch)) {
    auto line = rx.feed(static_cast<uint8_t>(ch));
    if (!line.has_value()) {
      continue;
    }
    if (SpaNetParser::classify_message(*line) == MessageType::kStateUpdate) {
      store.update(*line);
    }
  }

  const auto &state = store.get_state();
  EXPECT_EQ(state.controller_status.software_version, "V6.21.12.13");
  EXPECT_EQ(state.controller_status.model, "SVM1");
  EXPECT_EQ(state.controller_status.serial_number, "21460001-20000999");
  ASSERT_TRUE(state.controller_status.current_time.has_value());
  EXPECT_EQ(state.controller_status.current_time.value(), 1778453196);
  ASSERT_TRUE(state.temperatures.water_c.has_value());
  ASSERT_TRUE(state.temperatures.setpoint_c.has_value());
  EXPECT_NEAR(state.temperatures.water_c.value(), 39.4f, 0.01f);
  EXPECT_NEAR(state.temperatures.setpoint_c.value(), 39.0f, 0.01f);
}

TEST(ControllerStatusIntegrationTest, CurrentTimeEmptyWhenOnlyR3Received) {
  RegisterStore store;
  store.update(",R3,10,20,30,40,50,SW V6 21 12 13,SVM1,21460001,20000999,:");

  const auto &state = store.get_state();
  EXPECT_EQ(state.controller_status.software_version, "V6.21.12.13");
  EXPECT_FALSE(state.controller_status.current_time.has_value());
}

}  // namespace esphome::spanet::tests

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
