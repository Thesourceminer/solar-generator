#include "frame_tracker.h"

FrameTracker::Result FrameTracker::onFrame(const uint8_t* data, size_t len, uint32_t nowMs) {
  if (data == nullptr || len != kStatusLen) return WrongLength;
  StatusRecord r;
  if (!decodeStatus(data, len, r)) {  // right length, other version
    versionProblem_ = true;
    return BadVersion;
  }
  rec_ = r;
  has_ = true;
  versionProblem_ = false;
  goodAtMs_ = nowMs;
  return Ok;
}

uint32_t FrameTracker::msSinceGood(uint32_t nowMs) const {
  if (!has_) return 0x7fffffffUL;
  return nowMs - goodAtMs_;
}
