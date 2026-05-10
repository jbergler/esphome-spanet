#include <gtest/gtest.h>

#include "../../components/spanet/spanet_parser.h"

namespace esphome::spanet::tests {

// ── classify_message ─────────────────────────────────────────────────────────

TEST(ClassifyMessageTest, RfStartLineIsStateUpdate) {
  EXPECT_EQ(SpaNetParser::classify_message("RF:,R2:0,0"), MessageType::kStateUpdate);
  EXPECT_EQ(SpaNetParser::classify_message("RF,R2:0,0"), MessageType::kStateUpdate);
}

TEST(ClassifyMessageTest, StandaloneRegisterLineIsStateUpdate) {
  EXPECT_EQ(SpaNetParser::classify_message("R3:10,20,30"), MessageType::kStateUpdate);
  EXPECT_EQ(SpaNetParser::classify_message("RA:1,2,3"), MessageType::kStateUpdate);
}

TEST(ClassifyMessageTest, SAndWPrefixedLinesAreAck) {
  EXPECT_EQ(SpaNetParser::classify_message("S22-OK"), MessageType::kAck);
  EXPECT_EQ(SpaNetParser::classify_message("W40:380"), MessageType::kAck);
}

TEST(ClassifyMessageTest, UnrecognisedLinesAreUnknown) {
  EXPECT_EQ(SpaNetParser::classify_message("HELLO"), MessageType::kUnknown);
  EXPECT_EQ(SpaNetParser::classify_message(""), MessageType::kUnknown);
}

// ── parse_register_line ───────────────────────────────────────────────────────

TEST(ParseRegisterLineTest, ParsesRfStartLineWithEmbeddedRegister) {
  auto result = SpaNetParser::parse_register_line("RF:,R2:1,2,3");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first, "R2");
  ASSERT_EQ(result->second.size(), 3u);
  EXPECT_EQ(result->second[0], "1");
  EXPECT_EQ(result->second[2], "3");
}

TEST(ParseRegisterLineTest, ParsesStandaloneRegisterLine) {
  auto result = SpaNetParser::parse_register_line("R3:10,20,30,40,50,SW V3.1,SVM1,SN123,SN456");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->first, "R3");
  ASSERT_GE(result->second.size(), 9u);
  EXPECT_EQ(result->second[5], "SW V3.1");
  EXPECT_EQ(result->second[6], "SVM1");
}

TEST(ParseRegisterLineTest, ReturnsNulloptForNonRegisterLine) {
  EXPECT_FALSE(SpaNetParser::parse_register_line("HELLO").has_value());
  EXPECT_FALSE(SpaNetParser::parse_register_line("S22-OK").has_value());
  EXPECT_FALSE(SpaNetParser::parse_register_line("").has_value());
}

// ── RfRegisterStore ───────────────────────────────────────────────────────────

TEST(RfRegisterStoreTest, ReturnsNulloptBeforeR3IsReceived) {
  RfRegisterStore store;
  store.update("R2:1,2,3");
  EXPECT_FALSE(store.controller_identity().has_value());
}

TEST(RfRegisterStoreTest, ExtractsIdentityAfterR3Line) {
  RfRegisterStore store;
  store.update("R3:10,20,30,40,50,SW V3.1,SVM1,SN123,SN456");

  auto identity = store.controller_identity();
  ASSERT_TRUE(identity.has_value());
  EXPECT_EQ(identity->software_version, "SW V3.1");
  EXPECT_EQ(identity->model, "SVM1");
  EXPECT_EQ(identity->serial_number_1, "SN123");
  EXPECT_EQ(identity->serial_number_2, "SN456");
}

TEST(RfRegisterStoreTest, PartialUpdateOnlyChangesOneRegister) {
  RfRegisterStore store;
  store.update("R3:10,20,30,40,50,SW V3.1,SVM1,SN123,SN456");
  store.update("R2:1,2,3");  // unrelated register; R3 identity must survive

  EXPECT_TRUE(store.controller_identity().has_value());
}

TEST(RfRegisterStoreTest, OverwritesRegisterOnDuplicateLine) {
  RfRegisterStore store;
  store.update("R3:10,20,30,40,50,SW V3.1,OLD_MODEL,SN123,SN456");
  store.update("R3:10,20,30,40,50,SW V3.2,NEW_MODEL,SN123,SN456");

  auto identity = store.controller_identity();
  ASSERT_TRUE(identity.has_value());
  EXPECT_EQ(identity->model, "NEW_MODEL");
  EXPECT_EQ(identity->software_version, "SW V3.2");
}

TEST(RfRegisterStoreTest, ReturnsNulloptWhenR3HasTooFewFields) {
  RfRegisterStore store;
  store.update("R3:10,20");  // only 2 fields, need at least 9
  EXPECT_FALSE(store.controller_identity().has_value());
}

TEST(RfRegisterStoreTest, UpdateReturnsFalseForNonRegisterLine) {
  RfRegisterStore store;
  EXPECT_FALSE(store.update("HELLO"));
  EXPECT_FALSE(store.update("S22-OK"));
}

TEST(RfRegisterStoreTest, UpdateReturnsTrueForValidLine) {
  RfRegisterStore store;
  EXPECT_TRUE(store.update("R3:10,20,30,40,50,SW V3.1,SVM1,SN123,SN456"));
  EXPECT_TRUE(store.update("RF:,R2:1,2,3"));
}

TEST(RfRegisterStoreTest, TypedRegisterDecodesR3Identity) {
  RfRegisterStore store;
  store.update("R3:10,20,30,40,50,SW V3.1,SVM1,SN123,SN456");

  auto typed = store.typed_register("R3");
  ASSERT_TRUE(typed.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterR3>(*typed));

  const auto &r3 = std::get<RegisterR3>(*typed);
  EXPECT_EQ(r3.software_version, "SW V3.1");
  EXPECT_EQ(r3.model, "SVM1");
}

TEST(RfRegisterStoreTest, TypedRegisterDecodesKnownAndUnknownTypes) {
  RfRegisterStore store;
  store.update(
      "R4:NORM,0,0,0,Heat,00:00:00,05:41:31,0.4,0.2,1234,5678,11,22,3,4,5,6,7,8,9,10,11,12,13,14");
  store.update("R9:100,200,300,400,500,600,700,800,900,1000,1100,1200");

  auto r4 = store.typed_register("R4");
  ASSERT_TRUE(r4.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterR4>(*r4));
  EXPECT_EQ(std::get<RegisterR4>(*r4).mode, "NORM");
  EXPECT_EQ(std::get<RegisterR4>(*r4).power, "1234");
  EXPECT_EQ(std::get<RegisterR4>(*r4).vari_percent, "14");

  auto r9 = store.typed_register("R9");
  ASSERT_TRUE(r9.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterR9>(*r9));
  EXPECT_EQ(std::get<RegisterR9>(*r9).fault_code, "100");
}

TEST(RfRegisterStoreTest, ForEachTypedRegisterHandlesMultipleTypesDynamically) {
  RfRegisterStore store;
  store.update("R2:1,2,3,4,5,06,07,08,09,10,2026,12,13,1,0,14,15,16,17,18,21,22,23,24,25,26,27,28,29");
  store.update("R3:10,20,30,40,50,SW V3.1,SVM1,SN123,SN456");
  store.update(
      "R4:NORM,0,0,0,Heat,00:00:00,05:41:31,0.4,0.2,1234,5678,11,22,3,4,5,6,7,8,9,10,11,12,13,14");
  store.update("RA:11,12,13,14,15,16,17,18,19,20,21,22");

  int r2_count = 0;
  int r3_count = 0;
  int r4_count = 0;
  int ra_count = 0;

  store.for_each_typed_register([&](const AnyRegisterLine &line) {
    if (std::holds_alternative<RegisterR2>(line)) {
      r2_count++;
    } else if (std::holds_alternative<RegisterR3>(line)) {
      r3_count++;
    } else if (std::holds_alternative<RegisterR4>(line)) {
      r4_count++;
    } else if (std::holds_alternative<RegisterRA>(line)) {
      ra_count++;
    }
  });

  EXPECT_EQ(r2_count, 1);
  EXPECT_EQ(r3_count, 1);
  EXPECT_EQ(r4_count, 1);
  EXPECT_EQ(ra_count, 1);
}

TEST(RfRegisterStoreTest, TypedRegisterDecodesR2Semantics) {
  RfRegisterStore store;
  store.update("R2:1,2,3,4,5,06,07,08,09,10,2026,12,13,1,0,14,15,16,17,18,21,22,23,24,25,26,27,28,29");

  auto typed = store.typed_register("R2");
  ASSERT_TRUE(typed.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterR2>(*typed));

  const auto &r2 = std::get<RegisterR2>(*typed);
  EXPECT_EQ(r2.mains_current, "1");
  EXPECT_EQ(r2.spa_time_hour, "06");
  EXPECT_EQ(r2.spa_time_year, "2026");
  EXPECT_EQ(r2.relay_9, "29");
}

TEST(RfRegisterStoreTest, TypedRegisterReturnsNulloptForIncompleteR2R4) {
  RfRegisterStore store;
  store.update("R2:1,2,3");
  store.update("R4:1,2,3");

  auto r2 = store.typed_register("R2");
  auto r4 = store.typed_register("R4");
  EXPECT_FALSE(r2.has_value());
  EXPECT_FALSE(r4.has_value());
}

// ── RegisterR5 parsing tests ──────────────────────────────────────────────────

TEST(RegisterR5Test, ParsesRealPayload) {
  RfRegisterStore store;
  store.update("R5:0,1,0,5,0,0,0,0,0,0,0,0,1,0,394,0,26,0,4,0,0,0,1,2,6,6");

  auto typed = store.typed_register("R5");
  ASSERT_TRUE(typed.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterR5>(*typed));

  const auto &r5 = std::get<RegisterR5>(*typed);
  EXPECT_EQ(r5.status_0, "0");
  EXPECT_EQ(r5.status_1, "1");
  EXPECT_EQ(r5.status_3, "5");
  EXPECT_EQ(r5.status_12, "1");
  EXPECT_EQ(r5.status_25, "6");
}

// ── RegisterR6 parsing tests ──────────────────────────────────────────────────

TEST(RegisterR6Test, ParsesRealPayload) {
  RfRegisterStore store;
  store.update("R6:5,3,1,1,5,1,4,390,1,0,3584,5120,31,96,5632,5918,1792,1792,0,30,0,0,0,0,1,5,0,410");

  auto typed = store.typed_register("R6");
  ASSERT_TRUE(typed.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterR6>(*typed));

  const auto &r6 = std::get<RegisterR6>(*typed);
  EXPECT_EQ(r6.clean_cycle, "5");
  EXPECT_EQ(r6.vari_value, "3");
  EXPECT_EQ(r6.brightness, "1");
  EXPECT_EQ(r6.current_color, "1");
  EXPECT_EQ(r6.color_mode, "5");
  EXPECT_EQ(r6.filt_block_hrs, "390");
  EXPECT_EQ(r6.gas, "410");
}

// ── RegisterR7 parsing tests ──────────────────────────────────────────────────

TEST(RegisterR7Test, ParsesRealPayload) {
  RfRegisterStore store;
  store.update("R7:1792,0,1,0,1,0,0,6,2,2023,250,217,237,226,280,125,136,1,0,0,0,23,200,1,0,1,31,50,50,100,5");

  auto typed = store.typed_register("R7");
  ASSERT_TRUE(typed.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterR7>(*typed));

  const auto &r7 = std::get<RegisterR7>(*typed);
  EXPECT_EQ(r7.wcln_time, "1792");
  EXPECT_EQ(r7.temperature_units, "0");
  EXPECT_EQ(r7.ozone_off, "1");
  EXPECT_EQ(r7.v_max, "6");
  EXPECT_EQ(r7.hpmp, "23");
  EXPECT_EQ(r7.unknown_30, "5");
}

// ── RegisterR9 parsing tests ──────────────────────────────────────────────────

TEST(RegisterR9Test, ParsesRealPayload) {
  RfRegisterStore store;
  store.update("R9:F1,1023,5667,5,0,236,9999,455,46,0,255,27516");

  auto typed = store.typed_register("R9");
  ASSERT_TRUE(typed.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterR9>(*typed));

  const auto &r9 = std::get<RegisterR9>(*typed);
  EXPECT_EQ(r9.fault_code, "F1");
  EXPECT_EQ(r9.accum_1, "1023");
  EXPECT_EQ(r9.accum_2, "5667");
  EXPECT_EQ(r9.accum_11, "27516");
}

// ── RegisterRA parsing tests ──────────────────────────────────────────────────

TEST(RegisterRATest, ParsesRealPayload) {
  RfRegisterStore store;
  store.update("RA:F2,880,5679,4,0,231,9999,496,47,0,200,380");

  auto typed = store.typed_register("RA");
  ASSERT_TRUE(typed.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterRA>(*typed));

  const auto &ra = std::get<RegisterRA>(*typed);
  EXPECT_EQ(ra.fault_code, "F2");
  EXPECT_EQ(ra.accum_1, "880");
  EXPECT_EQ(ra.accum_2, "5679");
  EXPECT_EQ(ra.accum_11, "380");
}

// ── RegisterRB parsing tests ──────────────────────────────────────────────────

TEST(RegisterRBTest, ParsesRealPayload) {
  RfRegisterStore store;
  store.update("RB:F3,879,5890,4,0,242,9999,208,46,0,255,380");

  auto typed = store.typed_register("RB");
  ASSERT_TRUE(typed.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterRB>(*typed));

  const auto &rb = std::get<RegisterRB>(*typed);
  EXPECT_EQ(rb.fault_code, "F3");
  EXPECT_EQ(rb.accum_1, "879");
  EXPECT_EQ(rb.accum_2, "5890");
  EXPECT_EQ(rb.accum_11, "380");
}

// ── RegisterRC parsing tests ──────────────────────────────────────────────────

TEST(RegisterRCTest, ParsesRealPayload) {
  RfRegisterStore store;
  store.update("RC:0,0,0,0,0,0,0,0,0,2,0,0,0,0");

  auto typed = store.typed_register("RC");
  ASSERT_TRUE(typed.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterRC>(*typed));

  const auto &rc = std::get<RegisterRC>(*typed);
  EXPECT_EQ(rc.outlet_0, "0");
  EXPECT_EQ(rc.outlet_9, "2");
  EXPECT_EQ(rc.outlet_13, "0");
}

// ── RegisterRE parsing tests ──────────────────────────────────────────────────

TEST(RegisterRETest, ParsesRealPayload) {
  RfRegisterStore store;
  store.update("RE:0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,-4,13,30,8,5,1");

  auto typed = store.typed_register("RE");
  ASSERT_TRUE(typed.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterRE>(*typed));

  const auto &re = std::get<RegisterRE>(*typed);
  EXPECT_EQ(re.param_0, "0");
  EXPECT_EQ(re.param_16, "1");
  EXPECT_EQ(re.param_24, "-4");
  EXPECT_EQ(re.param_29, "1");
}

// ── RegisterRG parsing tests ──────────────────────────────────────────────────

TEST(RegisterRGTest, ParsesRealPayload) {
  RfRegisterStore store;
  store.update("RG:1,1,1,1,1,1,0-,1-2-0324,1-1-01,0-,0-,0,0,0,1808");

  auto typed = store.typed_register("RG");
  ASSERT_TRUE(typed.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterRG>(*typed));

  const auto &rg = std::get<RegisterRG>(*typed);
  EXPECT_EQ(rg.pump_1, "1");
  EXPECT_EQ(rg.pump_6, "1");
  EXPECT_EQ(rg.pump_7, "0-");
  EXPECT_EQ(rg.pump_8, "1-2-0324");
  EXPECT_EQ(rg.pump_14, "1808");
}

// ── MultiRegister payload handling ────────────────────────────────────────────

TEST(RfRegisterStoreTest, HandlesCompleteRealRfPayload) {
  RfRegisterStore store;

  // Update with real RF payload data (all registers from user input)
  store.update("RF:,R2:0,235,41,58,0,10,35,33,11,5,2026,375,9999,1,0,685,123,0,6000,342129,42284,40242,44,0,0,0,650,39660,42483,122");
  store.update("R3:10,1,4,4,4,SW V6 21 12 13,SVM1,21460001,20000999,0,1,0,0,0,0,NA,1,0,414,Auto,650,0,7,7,0,0,0");
  store.update("R4:NORM,0,0,0,1,1349,0,4,20,0,0,0,0,0,0,0,0,3,0,100,0,1359,4,80,100,0,0,4");
  store.update("R5:0,1,0,5,0,0,0,0,0,0,0,0,1,0,394,0,26,0,4,0,0,0,1,2,6,6");
  store.update("R6:5,3,1,1,5,1,4,390,1,0,3584,5120,31,96,5632,5918,1792,1792,0,30,0,0,0,0,1,5,0,410");
  store.update("R7:1792,0,1,0,1,0,0,6,2,2023,250,217,237,226,280,125,136,1,0,0,0,23,200,1,0,1,31,50,50,100,5");
  store.update("R9:F1,1023,5667,5,0,236,9999,455,46,0,255,27516");
  store.update("RA:F2,880,5679,4,0,231,9999,496,47,0,200,380");
  store.update("RB:F3,879,5890,4,0,242,9999,208,46,0,255,380");
  store.update("RC:0,0,0,0,0,0,0,0,0,2,0,0,0,0");
  store.update("RE:0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,-4,13,30,8,5,1");
  store.update("RG:1,1,1,1,1,1,0-,1-2-0324,1-1-01,0-,0-,0,0,0,1808");

  // Verify controller identity from R3
  auto identity = store.controller_identity();
  ASSERT_TRUE(identity.has_value());
  EXPECT_EQ(identity->software_version, "SW V6 21 12 13");
  EXPECT_EQ(identity->model, "SVM1");
  EXPECT_EQ(identity->serial_number_1, "21460001");
  EXPECT_EQ(identity->serial_number_2, "20000999");

  // Verify all registers are decoded and accessible
  int typed_count = 0;
  store.for_each_typed_register([&](const AnyRegisterLine &line) {
    EXPECT_FALSE(std::holds_alternative<UnknownRegisterLine>(line));
    typed_count++;
  });

  // Should have 12 successfully decoded registers
  EXPECT_EQ(typed_count, 12);
}

}  // namespace esphome::spanet::tests

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}