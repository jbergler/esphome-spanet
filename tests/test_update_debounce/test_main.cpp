#include <gtest/gtest.h>

#include "../../components/spanet/update_debounce.h"

namespace esphome::spanet::tests {

namespace {
uint32_t g_fake_now_ms = 0;

uint32_t fake_now_ms() { return g_fake_now_ms; }
} // namespace

TEST(UpdateDebounceGateTest, StartsDisarmed) {
  UpdateDebounceGate gate{50, &fake_now_ms};
  EXPECT_FALSE(gate.is_armed());
}

TEST(UpdateDebounceGateTest, ArmsOnceUntilDisarmed) {
  UpdateDebounceGate gate{50, &fake_now_ms};

  g_fake_now_ms = 100;
  EXPECT_TRUE(gate.try_arm());
  EXPECT_TRUE(gate.is_armed());

  g_fake_now_ms = 120;
  EXPECT_FALSE(gate.try_arm());
  EXPECT_TRUE(gate.is_armed());

  gate.disarm();
  EXPECT_FALSE(gate.is_armed());

  g_fake_now_ms = 130;
  EXPECT_TRUE(gate.try_arm());
}

TEST(UpdateDebounceGateTest, AutoExpiresAfterTimeout) {
  UpdateDebounceGate gate{50, &fake_now_ms};

  g_fake_now_ms = 100;
  EXPECT_TRUE(gate.try_arm());

  g_fake_now_ms = 149;
  EXPECT_FALSE(gate.try_arm());

  g_fake_now_ms = 150;
  EXPECT_TRUE(gate.try_arm());

  g_fake_now_ms = 170;
  EXPECT_FALSE(gate.try_arm());

  g_fake_now_ms = 201;
  EXPECT_TRUE(gate.try_arm());
}

} // namespace esphome::spanet::tests

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
