// Box-screen backlight: full for 60 s after a touch, dim until 5 min, then off (spec section 9).
#pragma once
#include <stdint.h>

enum BacklightLevel { BACKLIGHT_FULL, BACKLIGHT_DIM, BACKLIGHT_OFF };
static const uint32_t kDimAfterMs = 60UL * 1000UL;
static const uint32_t kOffAfterMs = 5UL * 60UL * 1000UL;
static const uint8_t kDimPercent = 30;

BacklightLevel backlightFor(uint32_t msSinceTouch);
// ELECROW V1.1-V1.3 board controller (I2C 0x30): 0 = brightest, 245 = off. `percent` 0-100 (clamped).
uint8_t stc8Brightness(uint8_t percent);
