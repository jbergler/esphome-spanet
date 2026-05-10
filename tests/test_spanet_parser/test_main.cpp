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
  store.update("R9:100,200");

  auto r4 = store.typed_register("R4");
  ASSERT_TRUE(r4.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterR4>(*r4));
  EXPECT_EQ(std::get<RegisterR4>(*r4).mode, "NORM");
  EXPECT_EQ(std::get<RegisterR4>(*r4).power, "1234");
  EXPECT_EQ(std::get<RegisterR4>(*r4).vari_percent, "14");

  auto r9 = store.typed_register("R9");
  ASSERT_TRUE(r9.has_value());
  ASSERT_TRUE(std::holds_alternative<UnknownRegisterLine>(*r9));
  EXPECT_EQ(std::get<UnknownRegisterLine>(*r9).label, "R9");
}

TEST(RfRegisterStoreTest, ForEachTypedRegisterHandlesMultipleTypesDynamically) {
  RfRegisterStore store;
  store.update("R2:1,2,3,4,5,06,07,08,09,10,2026,12,13,1,0,14,15,16,17,18,21,22,23,24,25,26,27,28,29");
  store.update("R3:10,20,30,40,50,SW V3.1,SVM1,SN123,SN456");
  store.update(
      "R4:NORM,0,0,0,Heat,00:00:00,05:41:31,0.4,0.2,1234,5678,11,22,3,4,5,6,7,8,9,10,11,12,13,14");
  store.update("RA:11,12");

  int r2_count = 0;
  int r3_count = 0;
  int r4_count = 0;
  int unknown_count = 0;

  store.for_each_typed_register([&](const AnyRegisterLine &line) {
    if (std::holds_alternative<RegisterR2>(line)) {
      r2_count++;
    } else if (std::holds_alternative<RegisterR3>(line)) {
      r3_count++;
    } else if (std::holds_alternative<RegisterR4>(line)) {
      r4_count++;
    } else if (std::holds_alternative<UnknownRegisterLine>(line)) {
      unknown_count++;
    }
  });

  EXPECT_EQ(r2_count, 1);
  EXPECT_EQ(r3_count, 1);
  EXPECT_EQ(r4_count, 1);
  EXPECT_EQ(unknown_count, 1);
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

}  // namespace esphome::spanet::tests

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}