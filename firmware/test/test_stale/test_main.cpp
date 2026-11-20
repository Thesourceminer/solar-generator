#include <unity.h>
#include "stale.h"

void setUp() {}
void tearDown() {}

static void test_never_read_is_stale() { TEST_ASSERT_TRUE(groupStale(255, 0)); }

static void test_fresh_reading_is_not_stale() {
  TEST_ASSERT_FALSE(groupStale(0, 0));
  TEST_ASSERT_FALSE(groupStale(4, 5000));  // 4 + 5 = 9 s
}

static void test_age_plus_time_since_frame_over_10s_is_stale() {
  TEST_ASSERT_TRUE(groupStale(4, 7000));   // 4 + 7 = 11 s
  TEST_ASSERT_TRUE(groupStale(0, 10001));  // frames stopped
  TEST_ASSERT_FALSE(groupStale(0, 10000));
}

static void test_link_down_after_10s_without_frames() {
  TEST_ASSERT_FALSE(linkDown(10000));
  TEST_ASSERT_TRUE(linkDown(10001));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_never_read_is_stale);
  RUN_TEST(test_fresh_reading_is_not_stale);
  RUN_TEST(test_age_plus_time_since_frame_over_10s_is_stale);
  RUN_TEST(test_link_down_after_10s_without_frames);
  return UNITY_END();
}
