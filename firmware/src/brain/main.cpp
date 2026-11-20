// Brain (ESP32-S3 in the controller case). Slice 1: demo data -> status store at 1 Hz -> phone (BLE) + display (ESP-NOW).
// Commands from the phone and from the display's touch screen both land in the same store.
#include <Arduino.h>

#include "ble_link.h"
#include "demo_source.h"
#include "espnow_link.h"
#include "status_store.h"

static StatusStore store;
static DemoSource demo;
static uint32_t lastTick = 0;
static QueueHandle_t displayCommands = nullptr;

static void onDisplayFrame(const uint8_t* data, size_t len) {
  Command c;
  if (parseCommand(data, len, c)) xQueueSend(displayCommands, &c, 0);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Solar Generator brain (slice 1, demo data)");
  displayCommands = xQueueCreate(8, sizeof(Command));
  if (!espnowBegin(onDisplayFrame)) Serial.println("ESP-NOW: start failed");
  bleBegin();
}

void loop() {
  bleApplyPending(store);
  Command c;
  while (xQueueReceive(displayCommands, &c, 0) == pdTRUE) store.apply(c);

  const uint32_t now = millis();
  if (now - lastTick >= 1000) {
    lastTick = now;
    store.tickSecond();
    demo.step(store, now);
    const StatusRecord r = store.snapshot();
    bleNotify(r);
    uint8_t bytes[kStatusLen];
    encodeStatus(r, bytes);
    espnowSend(bytes, kStatusLen);
  }
  delay(5);
}
