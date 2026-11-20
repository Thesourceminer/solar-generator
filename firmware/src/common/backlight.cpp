#include "backlight.h"

BacklightLevel backlightFor(uint32_t msSinceTouch) {
  if (msSinceTouch > kOffAfterMs) return BACKLIGHT_OFF;
  if (msSinceTouch > kDimAfterMs) return BACKLIGHT_DIM;
  return BACKLIGHT_FULL;
}

uint8_t stc8Brightness(uint8_t percent) {
  const uint32_t p = percent > 100 ? 100 : percent;
  return static_cast<uint8_t>(245 - (p * 245 + 50) / 100);
}
