#include <fstream>
#include <string>

#include <gtest/gtest.h>

#include "../../components/spanet/spanet_parser.h"
#include "../../components/spanet/spanet_state.h"
#include "../../components/spanet/uart_rx_buffer.h"

namespace esphome::spanet::tests {

// Feed the full example-1.txt payload through UartRxBuffer → SpaNetParser →
// RegisterStore and verify the normalized controller status is correct.
TEST(ControllerStatusIntegrationTest, ParsesFullExamplePayload) {
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

  // Identity from R3
  EXPECT_EQ(state.controller_status.software_version, "SW V6 21 12 13");
  EXPECT_EQ(state.controller_status.model, "SVM1");
  EXPECT_EQ(state.controller_status.serial_number_1, "21460001");
  EXPECT_EQ(state.controller_status.serial_number_2, "20000999");

  // Datetime from R2 normalized to UTC epoch
  ASSERT_TRUE(state.controller_status.current_time.has_value());
  EXPECT_EQ(state.controller_status.current_time.value(), 1778453196);
}

TEST(ControllerStatusIntegrationTest, DatetimeEmptyWhenOnlyR3Received) {
  RegisterStore store;
  store.update(",R3,10,20,30,40,50,SW V6 21 12 13,SVM1,21460001,20000999,:");

  const auto &state = store.get_state();
  EXPECT_EQ(state.controller_status.software_version, "SW V6 21 12 13");

  // No R2 received — epoch must be empty
  EXPECT_FALSE(state.controller_status.current_time.has_value());
}

}  // namespace esphome::spanet::tests

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
