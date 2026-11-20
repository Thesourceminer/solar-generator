#include "stale.h"

bool groupStale(uint8_t age, uint32_t msSinceFrame) {
  if (age == 255) return true;
  return static_cast<uint32_t>(age) * 1000UL + msSinceFrame > kStaleAfterMs;
}

bool linkDown(uint32_t msSinceFrame) { return msSinceFrame > kStaleAfterMs; }
