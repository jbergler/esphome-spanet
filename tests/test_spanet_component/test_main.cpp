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

  EXPECT_EQ(manager.enqueue(Command{
                .kind = CommandKind::kSetpointWrite,
                .payload = "W40:390",
                .expected_ack = "390",
                .timeout_ms = 1500,
            }),
            EnqueueResult::kEnqueued);

  EXPECT_EQ(manager.enqueue(Command{
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
  manager.maybe_send_next_();

  ASSERT_EQ(writes.size(), 2u);
  EXPECT_EQ(writes[1], "RF\n");
}

TEST(CommandQueueTest, TimeoutAdvancesQueue) {
  std::vector<std::string> writes;
  uint32_t now_ms = 100;

  CommandQueue manager([&](const std::string &payload) { writes.push_back(payload); }, [&]() { return now_ms; }, 8);

  manager.enqueue(Command{
      .kind = CommandKind::kSetpointWrite,
      .payload = "W40:390",
      .expected_ack = "390",
      .timeout_ms = 500,
  });
  manager.enqueue(Command{
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
  EXPECT_EQ(timed_out.command.kind, CommandKind::kSetpointWrite);

  ASSERT_EQ(writes.size(), 2u);
  EXPECT_EQ(writes[1], "RF\n");
}

TEST(CommandQueueTest, UnmatchedAckKeepsInFlightCommand) {
  std::vector<std::string> writes;
  uint32_t now_ms = 100;
  CommandQueue manager([&](const std::string &payload) { writes.push_back(payload); }, [&]() { return now_ms; }, 8);
  manager.enqueue(Command{
      .kind = CommandKind::kSetpointWrite,
      .payload = "W40:390",
      .expected_ack = "390",
      .timeout_ms = 1500,
  });
  manager.enqueue(Command{
      .kind = CommandKind::kRfPoll,
      .payload = "RF",
      .expected_ack = std::nullopt,
      .timeout_ms = 0,
  });
  ASSERT_EQ(writes.size(), 1u);
  EXPECT_EQ(manager.acknowledge("999", nullptr), AckResult::kUnmatchedAck);
  ASSERT_EQ(writes.size(), 1u);  // RF not yet sent; in-flight still pending
}

TEST(CommandQueueTest, LightColorAckAdvancesQueue) {
  std::vector<std::string> writes;
  uint32_t now_ms = 100;

  CommandQueue manager([&](const std::string &payload) { writes.push_back(payload); }, [&]() { return now_ms; }, 8);

  EXPECT_EQ(manager.enqueue(Command{
                .kind = CommandKind::kLightColor,
                .payload = "S10:12",
                .expected_ack = "12",
                .timeout_ms = 1500,
            }),
            EnqueueResult::kEnqueued);

  EXPECT_EQ(manager.enqueue(Command{
                .kind = CommandKind::kRfPoll,
                .payload = "RF",
                .expected_ack = std::nullopt,
                .timeout_ms = 0,
            }),
            EnqueueResult::kEnqueued);

  ASSERT_EQ(writes.size(), 1u);
  EXPECT_EQ(writes[0], "S10:12\n");

  InFlightCommand matched;
  EXPECT_EQ(manager.acknowledge("12", &matched), AckResult::kMatched);
  EXPECT_EQ(matched.command.kind, CommandKind::kLightColor);
  manager.maybe_send_next_();

  ASSERT_EQ(writes.size(), 2u);
  EXPECT_EQ(writes[1], "RF\n");
}

TEST(CommandQueueTest, TimeoutDoesNotFireBeforeBoundary) {
  std::vector<std::string> writes;
  uint32_t now_ms = 100;
  CommandQueue manager([&](const std::string &payload) { writes.push_back(payload); }, [&]() { return now_ms; }, 8);
  manager.enqueue(Command{
      .kind = CommandKind::kSetpointWrite,
      .payload = "W40:390",
      .expected_ack = "390",
      .timeout_ms = 500,
  });
  now_ms = 599;
  InFlightCommand timed_out;
  EXPECT_FALSE(manager.expire_timed_out(now_ms, &timed_out, nullptr));
}

TEST(CommandQueueTest, ZeroTimeoutNeverExpiresInFlightCommand) {
  std::vector<std::string> writes;
  uint32_t now_ms = 100;
  CommandQueue manager([&](const std::string &payload) { writes.push_back(payload); }, [&]() { return now_ms; }, 8);
  manager.enqueue(Command{
      .kind = CommandKind::kSetpointWrite,
      .payload = "W40:390",
      .expected_ack = "390",
      .timeout_ms = 0,  // No timeout
  });
  now_ms = 10000;
  InFlightCommand timed_out;
  EXPECT_FALSE(manager.expire_timed_out(now_ms, &timed_out, nullptr));
}

TEST(CommandQueueTest, StagedRfAckKeepsQueueBlockedUntilSentinel) {
  std::vector<std::string> writes;
  uint32_t now_ms = 100;
  CommandQueue manager([&](const std::string &payload) { writes.push_back(payload); }, [&]() { return now_ms; }, 8);

  manager.enqueue(Command{
      .kind = CommandKind::kRfPoll,
      .payload = "RF",
      .expected_ack = "RF:",
      .completion_predicate = [](const std::string &message) { return message == ",RG,1,:"; },
      .timeout_ms = 5000,
  });
  manager.enqueue(Command{
      .kind = CommandKind::kSetpointWrite,
      .payload = "W40:390",
      .expected_ack = "390",
      .timeout_ms = 1500,
  });

  ASSERT_EQ(writes.size(), 1u);
  EXPECT_EQ(writes[0], "RF\n");

  EXPECT_EQ(manager.acknowledge("RF:", nullptr), AckResult::kInProgress);
  ASSERT_EQ(writes.size(), 1u);

  EXPECT_EQ(manager.acknowledge(",R2,0,239,:", nullptr), AckResult::kInProgress);
  ASSERT_EQ(writes.size(), 1u);

  InFlightCommand matched;
  EXPECT_EQ(manager.acknowledge(",RG,1,:", &matched), AckResult::kMatched);
  EXPECT_EQ(matched.command.kind, CommandKind::kRfPoll);
  manager.maybe_send_next_();

  ASSERT_EQ(writes.size(), 2u);
  EXPECT_EQ(writes[1], "W40:390\n");
}

TEST(CommandQueueTest, StagedRfAllowsReSentinelCompletion) {
  std::vector<std::string> writes;
  uint32_t now_ms = 100;
  CommandQueue manager([&](const std::string &payload) { writes.push_back(payload); }, [&]() { return now_ms; }, 8);

  manager.enqueue(Command{
      .kind = CommandKind::kRfPoll,
      .payload = "RF",
      .expected_ack = "RF:",
      .completion_predicate =
          [](const std::string &message) { return message.rfind(",RE,", 0) == 0 || message.rfind(",RG,", 0) == 0; },
      .timeout_ms = 5000,
  });
  manager.enqueue(Command{
      .kind = CommandKind::kLightColor,
      .payload = "S10:12",
      .expected_ack = "12",
      .timeout_ms = 1500,
  });

  EXPECT_EQ(manager.acknowledge("RF:", nullptr), AckResult::kInProgress);
  EXPECT_EQ(manager.acknowledge(",R6,1,2,:", nullptr), AckResult::kInProgress);
  EXPECT_EQ(manager.acknowledge(",RE,0,0,:", nullptr), AckResult::kMatched);
  manager.maybe_send_next_();

  ASSERT_EQ(writes.size(), 2u);
  EXPECT_EQ(writes[1], "S10:12\n");
}

TEST(CommandQueueTest, StagedRfTimeoutAdvancesQueueAfterAckPhase) {
  std::vector<std::string> writes;
  uint32_t now_ms = 100;
  CommandQueue manager([&](const std::string &payload) { writes.push_back(payload); }, [&]() { return now_ms; }, 8);

  manager.enqueue(Command{
      .kind = CommandKind::kRfPoll,
      .payload = "RF",
      .expected_ack = "RF:",
      .completion_predicate = [](const std::string &message) { return message == ",RG,1,:"; },
      .timeout_ms = 500,
  });
  manager.enqueue(Command{
      .kind = CommandKind::kPumpWrite,
      .payload = "S15:1",
      .expected_ack = "1",
      .timeout_ms = 1500,
  });

  EXPECT_EQ(manager.acknowledge("RF:", nullptr), AckResult::kInProgress);

  now_ms = 600;
  InFlightCommand timed_out;
  uint32_t age_ms = 0;
  EXPECT_TRUE(manager.expire_timed_out(now_ms, &timed_out, &age_ms));
  EXPECT_EQ(timed_out.command.kind, CommandKind::kRfPoll);
  EXPECT_EQ(age_ms, 500u);

  ASSERT_EQ(writes.size(), 2u);
  EXPECT_EQ(writes[1], "S15:1\n");
}

}  // namespace esphome::spanet::tests

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
