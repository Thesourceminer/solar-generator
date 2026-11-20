// The one in-memory status that the phone link, the display link and (later) the gear readers share.
#pragma once
#include "protocol.h"

class StatusStore {
 public:
  // Applies a command from the phone or the display. Slice 1: the store itself confirms it.
  void apply(const Command& c);
  // Next record to send: seq + 1 (wraps 65535 -> 0), then a copy.
  StatusRecord snapshot();
  // Once a second: every group age + 1, saturating at 254; 255 (never read) stays 255.
  void tickSecond();
  StatusRecord& raw() { return r_; }

 private:
  StatusRecord r_;
};
