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
  EXPECT_EQ(SpaNetParser::classify_message(",R3,10,1,4,4,SW V6,:"), MessageType::kStateUpdate);
  EXPECT_EQ(SpaNetParser::classify_message(",RE,0,0,0,0,:*"), MessageType::kStateUpdate);
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
  auto result = SpaNetParser::parse_register_line("RF:,R2,0,239,40,81,0,10,46,36,11,5,2026,385,9999,1,0,674,127,0,6000,342132,42286,40243,44,0,0,0,650,39660,42484,126,:");
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterR2>(result.value()));
}

TEST(ParseRegisterLineTest, ParsesStandaloneRegisterLine) {
  auto result = SpaNetParser::parse_register_line(",R3,10,20,30,40,50,SW V3.1,SVM1,SN123,SN456,:");
  ASSERT_TRUE(result.has_value());
  ASSERT_TRUE(std::holds_alternative<RegisterR3>(result.value()));

  const auto &r3 = std::get<RegisterR3>(result.value());
  EXPECT_EQ(r3.software_version, "SW V3.1");
  EXPECT_EQ(r3.model, "SVM1");
}

TEST(ParseRegisterLineTest, ReturnsNulloptForNonRegisterLine) {
  EXPECT_FALSE(SpaNetParser::parse_register_line("HELLO").has_value());
  EXPECT_FALSE(SpaNetParser::parse_register_line("S22-OK").has_value());
  EXPECT_FALSE(SpaNetParser::parse_register_line("").has_value());
}

}  // namespace esphome::spanet::tests

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
