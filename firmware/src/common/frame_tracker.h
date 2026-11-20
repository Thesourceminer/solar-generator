// Keeps the box screen's latest good status record and when it arrived. Only a record that decodes is "good":
// a frame with another version or length never refreshes the link time, so old numbers turn "--" on schedule.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "protocol.h"

class FrameTracker {
 public:
  enum Result { Ok, WrongLength, BadVersion };

  Result onFrame(const uint8_t* data, size_t len, uint32_t nowMs);
  bool hasRecord() const { return has_; }
  const StatusRecord& record() const { return rec_; }
  // Milliseconds since the last good record; very large when none has arrived yet.
  uint32_t msSinceGood(uint32_t nowMs) const;
  // True after a frame with another protocol version, until the next good record.
  bool versionProblem() const { return versionProblem_; }

 private:
  StatusRecord rec_;
  bool has_ = false;
  bool versionProblem_ = false;
  uint32_t goodAtMs_ = 0;
};
