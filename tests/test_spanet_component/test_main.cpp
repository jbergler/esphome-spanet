#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "../../components/spanet/command_queue.h"

namespace esphome::spanet::tests {

TEST(CommandQueueTest, AckedCommandBlocksQueueUntilAck) {
  std::vector<std::string> writes;
  uint32_t now_ms = 100;

  CommandQueue manager([&](const std::string &payload) { writes.push_back(payload); }, [&]() { return now_ms; }, 8);

  EXPECT_EQ(manager.enqueue(QueuedCommand{
                .kind = CommandKind::kSetpointWrite,
                .payload = "W40:390",
                .expected_ack = "390",
                .timeout_ms = 1500,
            }),
            EnqueueResult::kEnqueued);

  EXPECT_EQ(manager.enqueue(QueuedCommand{
                .kind = CommandKind::kRfPoll,
                .payload = "RF",
                .expected_ack = std::nullopt,
                .timeout_ms = 0,
            }),
            EnqueueResult::kEnqueued);

  ASSERT_EQ(writes.size(), 1u);
  EXPECT_EQ(writes[0], "W40:390\n");

  InFlightCommand matched;
  EXPECT_EQ(manager.acknowledge("390", &matched), AckResult::kMatched);

  ASSERT_EQ(writes.size(), 2u);
  EXPECT_EQ(writes[1], "RF\n");
}

TEST(CommandQueueTest, TimeoutAdvancesQueue) {
  std::vector<std::string> writes;
  uint32_t now_ms = 100;

  CommandQueue manager([&](const std::string &payload) { writes.push_back(payload); }, [&]() { return now_ms; }, 8);

  manager.enqueue(QueuedCommand{
      .kind = CommandKind::kSetpointWrite,
      .payload = "W40:390",
      .expected_ack = "390",
      .timeout_ms = 500,
  });
  manager.enqueue(QueuedCommand{
      .kind = CommandKind::kRfPoll,
      .payload = "RF",
      .expected_ack = std::nullopt,
      .timeout_ms = 0,
  });

  now_ms = 600;
  InFlightCommand timed_out;
  uint32_t age_ms = 0;
  EXPECT_TRUE(manager.expire_timed_out(now_ms, &timed_out, &age_ms));
  EXPECT_EQ(age_ms, 500u);
  EXPECT_EQ(timed_out.kind, CommandKind::kSetpointWrite);

  ASSERT_EQ(writes.size(), 2u);
  EXPECT_EQ(writes[1], "RF\n");
}

}  // namespace esphome::spanet::tests

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
