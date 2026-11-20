#include <unity.h>
#include "backlight.h"
#include "frame_tracker.h"

void setUp() {}
void tearDown() {}

static void encodeTypical(uint8_t out[kStatusLen], uint8_t soc) {
  StatusRecord r;
  r.flags = FLAG_DEMO;
  r.battAge = 0;
  r.socPct = soc;
  encodeStatus(r, out);
}

static void test_good_frame_is_kept_and_stamped() {
  FrameTracker t;
  TEST_ASSERT_FALSE(t.hasRecord());
  uint8_t b[kStatusLen];
  encodeTypical(b, 87);
  TEST_ASSERT_EQUAL(FrameTracker::Ok, t.onFrame(b, kStatusLen, 5000));
  TEST_ASSERT_TRUE(t.hasRecord());
  TEST_ASSERT_EQUAL_UINT8(87, t.record().socPct);
  TEST_ASSERT_EQUAL_UINT32(1500, t.msSinceGood(6500));
  TEST_ASSERT_FALSE(t.versionProblem());
}

static void test_bad_version_does_not_refresh_link_time_and_flags_problem() {
  FrameTracker t;
  uint8_t b[kStatusLen];
  encodeTypical(b, 87);
  t.onFrame(b, kStatusLen, 1000);
  b[0] = 2;
  TEST_ASSERT_EQUAL(FrameTracker::BadVersion, t.onFrame(b, kStatusLen, 9000));
  TEST_ASSERT_EQUAL_UINT8(87, t.record().socPct);
  TEST_ASSERT_EQUAL_UINT32(11000, t.msSinceGood(12000));  // still timed from the last GOOD frame
  TEST_ASSERT_TRUE(t.versionProblem());
  encodeTypical(b, 88);
  t.onFrame(b, kStatusLen, 12500);
  TEST_ASSERT_FALSE(t.versionProblem());
}

static void test_wrong_length_ignored() {
  FrameTracker t;
  uint8_t b[kStatusLen];
  encodeTypical(b, 50);
  TEST_ASSERT_EQUAL(FrameTracker::WrongLength, t.onFrame(b, 3, 100));
  TEST_ASSERT_FALSE(t.hasRecord());
}

static void test_never_received_counts_as_link_down() {
  FrameTracker t;
  TEST_ASSERT_TRUE(t.msSinceGood(0) > 10000);
}

static void test_stc8_brightness_scale() {
  TEST_ASSERT_EQUAL_UINT8(0, stc8Brightness(100));    // 0 = brightest (ELECROW)
  TEST_ASSERT_EQUAL_UINT8(245, stc8Brightness(0));    // 245 = off
  TEST_ASSERT_EQUAL_UINT8(171, stc8Brightness(30));
  TEST_ASSERT_EQUAL_UINT8(0, stc8Brightness(150));    // clamped
}

static void test_backlight_policy() {
  TEST_ASSERT_EQUAL(BACKLIGHT_FULL, backlightFor(0));
  TEST_ASSERT_EQUAL(BACKLIGHT_FULL, backlightFor(60000));
  TEST_ASSERT_EQUAL(BACKLIGHT_DIM, backlightFor(60001));
  TEST_ASSERT_EQUAL(BACKLIGHT_DIM, backlightFor(300000));
  TEST_ASSERT_EQUAL(BACKLIGHT_OFF, backlightFor(300001));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_good_frame_is_kept_and_stamped);
  RUN_TEST(test_bad_version_does_not_refresh_link_time_and_flags_problem);
  RUN_TEST(test_wrong_length_ignored);
  RUN_TEST(test_never_received_counts_as_link_down);
  RUN_TEST(test_stc8_brightness_scale);
  RUN_TEST(test_backlight_policy);
  return UNITY_END();
}
