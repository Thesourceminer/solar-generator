// The box screen: the same two pages as the phone app (status, swipe for controls), landscape 800 x 480.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include "protocol.h"

typedef void (*CommandSink)(const uint8_t* bytes, size_t len);

// Brings up the panel, touch, backlight and LVGL. Touches on the controls page call `sink` with command bytes.
void screenBegin(CommandSink sink);
// Redraws from the latest good record (nullptr = nothing received yet). `msSinceGood` = time since that record;
// `versionProblem` = the box sent another protocol version. Call about once a second.
void screenUpdate(const StatusRecord* r, uint32_t msSinceGood, bool versionProblem);
// Runs LVGL and the backlight timer; call every loop.
void screenLoop();
