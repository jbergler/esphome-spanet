#include <gtest/gtest.h>

#include "../../components/spanet/uart_rx_buffer.h"

namespace esphome::spanet::tests {

TEST(UartRxBufferTest, EmitsMessagesForRepeatedDummyFrames) {
  UartRxBuffer buffer;
  const std::string stream = "DUMMY\nDUMMY\nDUMMY\n";

  int emitted = 0;
  for (char ch : stream) {
    auto msg = buffer.feed(static_cast<uint8_t>(ch));
    if (msg.has_value()) {
      emitted++;
      EXPECT_EQ(msg.value(), "DUMMY");
    }
  }

  EXPECT_EQ(emitted, 3);
}

TEST(UartRxBufferTest, IgnoresCarriageReturn) {
  UartRxBuffer buffer;
  const std::string stream = "A\rB\n";

  std::optional<std::string> out;
  for (char ch : stream) {
    out = buffer.feed(static_cast<uint8_t>(ch));
  }

  ASSERT_TRUE(out.has_value());
  EXPECT_EQ(out.value(), "AB");
}

} // namespace esphome::spanet::tests

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
