// The 10-second "show --" rule, shared by the box screen (the phone app has the same rule in Swift).
#pragma once
#include <stdint.h>

static const uint32_t kStaleAfterMs = 10000;

// True when a group was never read (age 255) or its age plus the time since the last frame is over 10 s.
bool groupStale(uint8_t age, uint32_t msSinceFrame);
// True when no frame has arrived for over 10 s.
bool linkDown(uint32_t msSinceFrame);
