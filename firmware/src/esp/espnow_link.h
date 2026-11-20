// Brain <-> display link over ESP-NOW. Frames are the shared protocol bytes unchanged:
// brain -> display = the 36-byte status record, display -> brain = command bytes.
//
// TEMPORARY (slice 1): broadcast, unencrypted, accepts any sender. ESP-NOW can only encrypt unicast frames, so
// slice 2 replaces this with a one-time pairing (PIN-protected, like phone pairing) that stores the other board's
// MAC + a shared key and drops everything else. Nothing here may switch real gear until then.
#pragma once
#include <stddef.h>
#include <stdint.h>

// Tripwire: the slice that wires real gear defines REAL_GEAR; it must not build while this link is still open.
#define ESPNOW_INSECURE 1
#if defined(REAL_GEAR) && ESPNOW_INSECURE
#error "ESP-NOW is still unencrypted broadcast: add board pairing (slice 2) before building with REAL_GEAR"
#endif

typedef void (*EspNowFrameHandler)(const uint8_t* data, size_t len);

// Starts Wi-Fi in station mode on channel 1 (no network joined) and ESP-NOW. `onFrame` runs in the Wi-Fi task:
// copy the bytes and return quickly.
bool espnowBegin(EspNowFrameHandler onFrame);
bool espnowSend(const uint8_t* data, size_t len);
