// Believable, slowly drifting readings for demo mode (no gear connected).
#pragma once
#include <stdint.h>
#include "status_store.h"

class DemoSource {
 public:
  // Writes fresh readings (ages 0, demo flag set) for time `ms` since boot. Leaves switch states alone.
  void step(StatusStore& store, uint32_t ms);
};
