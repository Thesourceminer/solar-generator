#include <unity.h>
#include "demo_source.h"
#include "status_store.h"

void setUp() {}
void tearDown() {}

static Command cmd(uint8_t op, uint8_t a, uint8_t b = 0) {
  Command c;
  c.op = op;
  c.a = a;
  c.b = b;
  return c;
}

static void test_set_circuit_3_on_sets_on_and_confirmed_bits() {
  StatusStore s;
  s.apply(cmd(OP_SET_CIRCUIT, 3, 1));
  TEST_ASSERT_EQUAL_HEX8(0x04, s.raw().circOn);
  TEST_ASSERT_EQUAL_HEX8(0x04, s.raw().circConfirmed);
  s.apply(cmd(OP_SET_CIRCUIT, 3, 0));
  TEST_ASSERT_EQUAL_HEX8(0x00, s.raw().circOn);
  TEST_ASSERT_EQUAL_HEX8(0x04, s.raw().circConfirmed);
}

static void test_inverter_and_fan_commands() {
  StatusStore s;
  s.apply(cmd(OP_SET_INVERTER, 1));
  TEST_ASSERT_EQUAL_HEX8(INV_ON, s.raw().invBits & INV_ON);
  s.apply(cmd(OP_FAN_MODE, 1));
  s.apply(cmd(OP_FAN_PCT, 55));
  TEST_ASSERT_EQUAL_UINT8(1, s.raw().fanMode);
  TEST_ASSERT_EQUAL_UINT8(55, s.raw().fanPct);
  s.apply(cmd(OP_SET_INVERTER, 0));
  TEST_ASSERT_EQUAL_HEX8(0, s.raw().invBits & INV_ON);
}

static void test_ages_saturate_at_254_and_never_stays_255() {
  StatusStore s;
  s.raw().battAge = 0;
  s.raw().chgAge = 0;
  for (int i = 0; i < 300; i++) s.tickSecond();
  TEST_ASSERT_EQUAL_UINT8(254, s.raw().battAge);
  TEST_ASSERT_EQUAL_UINT8(254, s.raw().chgAge);
  TEST_ASSERT_EQUAL_UINT8(255, s.raw().invAge);
}

static void test_snapshot_seq_wraps() {
  StatusStore s;
  s.raw().seq = 65534;
  TEST_ASSERT_EQUAL_UINT16(65535, s.snapshot().seq);
  TEST_ASSERT_EQUAL_UINT16(0, s.snapshot().seq);
}

static void test_demo_step_gives_fresh_plausible_readings_and_keeps_switches() {
  StatusStore s;
  s.apply(cmd(OP_SET_CIRCUIT, 1, 1));
  DemoSource d;
  d.step(s, 123456);
  const StatusRecord& r = s.raw();
  TEST_ASSERT_TRUE(r.flags & FLAG_DEMO);
  TEST_ASSERT_EQUAL_UINT8(0, r.battAge);
  TEST_ASSERT_EQUAL_UINT8(0, r.chgAge);
  TEST_ASSERT_EQUAL_UINT8(0, r.fanAge);
  TEST_ASSERT_TRUE(r.socPct <= 100);
  TEST_ASSERT_TRUE(r.battCentiVolts >= 1200 && r.battCentiVolts <= 1460);
  TEST_ASSERT_EQUAL_UINT8(1, r.circReady);
  TEST_ASSERT_EQUAL_HEX8(0x01, r.circOn & 0x01);
}

static void test_demo_values_stay_in_range_over_a_full_cycle() {
  StatusStore s;
  DemoSource d;
  for (uint32_t ms = 0; ms <= 660000; ms += 500) {
    d.step(s, ms);
    const StatusRecord& r = s.raw();
    TEST_ASSERT_TRUE_MESSAGE(r.fanPct <= 100, "fanPct over 100");
    TEST_ASSERT_TRUE_MESSAGE(r.socPct <= 100, "socPct over 100");
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_demo_values_stay_in_range_over_a_full_cycle);
  RUN_TEST(test_set_circuit_3_on_sets_on_and_confirmed_bits);
  RUN_TEST(test_inverter_and_fan_commands);
  RUN_TEST(test_ages_saturate_at_254_and_never_stays_255);
  RUN_TEST(test_snapshot_seq_wraps);
  RUN_TEST(test_demo_step_gives_fresh_plausible_readings_and_keeps_switches);
  return UNITY_END();
}
