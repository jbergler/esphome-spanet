#include <fstream>
#include <string>

#include <gtest/gtest.h>

#include "../../components/spanet/spanet_parser.h"
#include "../../components/spanet/spanet_state.h"
#include "../../components/spanet/uart_rx_buffer.h"

namespace esphome::spanet::tests {

// Feed the full example-1.txt payload through UartRxBuffer → SpaNetParser →
// RfRegisterStore and verify SpaNetState recomputes the expected normalized
// controller status.
TEST(ControllerStatusIntegrationTest, ParsesFullExamplePayload) {
  std::ifstream file("tests/data/example-1.txt");
  ASSERT_TRUE(file.is_open()) << "Could not open tests/data/example-1.txt";

  UartRxBuffer rx{256};
  RfRegisterStore store;

  char ch;
  while (file.get(ch)) {
    auto line = rx.feed(static_cast<uint8_t>(ch));
    if (!line.has_value()) {
      continue;
    }
    if (SpaNetParser::classify_message(*line) == MessageType::kStateUpdate) {
      auto parsed = SpaNetParser::parse_register_line(*line);
      if (!parsed.has_value()) {
        continue;
      }
      ASSERT_TRUE(store.update_fields(parsed->first, parsed->second));
    }
  }

  SpaNetState state;
  ASSERT_TRUE(state.recompute_from(store));

  // Identity from R3
  EXPECT_EQ(state.controller.software_version, "SW V6 21 12 13");
  EXPECT_EQ(state.controller.model, "SVM1");
  EXPECT_EQ(state.controller.serial_number_1, "21460001");
  EXPECT_EQ(state.controller.serial_number_2, "20000999");

  // Datetime from R2 normalized to UTC epoch
  ASSERT_TRUE(state.controller.controller_epoch.has_value());
  EXPECT_EQ(state.controller.controller_epoch.value(), 1778496396);
}

TEST(ControllerStatusIntegrationTest, DatetimeEmptyWhenOnlyR3Received) {
  RfRegisterStore store;
  auto parsed = SpaNetParser::parse_register_line(",R3,10,20,30,40,50,SW V6 21 12 13,SVM1,21460001,20000999,:");
  ASSERT_TRUE(parsed.has_value());
  ASSERT_TRUE(store.update_fields(parsed->first, parsed->second));

  SpaNetState state;
  ASSERT_TRUE(state.recompute_from(store));
  EXPECT_EQ(state.controller.software_version, "SW V6 21 12 13");

  // No R2 received — epoch must be empty
  EXPECT_FALSE(state.controller.controller_epoch.has_value());
}

}  // namespace esphome::spanet::tests

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
