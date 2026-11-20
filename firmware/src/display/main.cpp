// Display (ELECROW CrowPanel Advance 4.3 in on the outside wall). Gets status records from the brain over ESP-NOW,
// draws them, and sends touches on the controls page back to the brain as command bytes.
#include <Arduino.h>

#include "espnow_link.h"
#include "frame_tracker.h"
#include "protocol.h"
#include "screen.h"

static portMUX_TYPE frameLock = portMUX_INITIALIZER_UNLOCKED;
static uint8_t frameBytes[kStatusLen];
static volatile bool frameNew = false;

static FrameTracker tracker;
static uint32_t lastRedraw = 0;

static void onFrame(const uint8_t* data, size_t len) {
  if (len != kStatusLen) return;  // command bytes from another display, or noise
  portENTER_CRITICAL(&frameLock);
  memcpy(frameBytes, data, kStatusLen);
  frameNew = true;
  portEXIT_CRITICAL(&frameLock);
}

static void sendCommand(const uint8_t* b, size_t n) { espnowSend(b, n); }

void setup() {
  Serial.begin(115200);
  Serial.println("Solar Generator display (slice 1)");
  screenBegin(sendCommand);
  if (!espnowBegin(onFrame)) Serial.println("ESP-NOW: start failed");
}

void loop() {
  uint8_t copy[kStatusLen];
  bool fresh = false;
  portENTER_CRITICAL(&frameLock);
  if (frameNew) {
    memcpy(copy, frameBytes, kStatusLen);
    frameNew = false;
    fresh = true;
  }
  portEXIT_CRITICAL(&frameLock);

  const uint32_t now = millis();
  // Only a frame that decodes refreshes the link time (a frame in another format never makes old numbers look live).
  if (fresh) tracker.onFrame(copy, kStatusLen, now);

  if (fresh || now - lastRedraw >= 1000) {
    lastRedraw = now;
    screenUpdate(tracker.hasRecord() ? &tracker.record() : nullptr, tracker.msSinceGood(now), tracker.versionProblem());
  }
  screenLoop();
  delay(2);
}
