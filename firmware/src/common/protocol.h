// Link protocol v1 (see shared/protocol.md). Plain C++, no Arduino, so it runs in the native tests.
#pragma once
#include <stddef.h>
#include <stdint.h>

static const size_t kStatusLen = 36;
static const uint8_t kStatusVersion = 1;

enum StatusFlags : uint8_t { FLAG_DEMO = 1, FLAG_PAIRING = 2, FLAG_LOW_BATT = 4, FLAG_CUT_PENDING = 8 };
enum InverterBits : uint8_t { INV_ON = 1, INV_ERR = 2 };
static const uint8_t kAgeNever = 255;

struct StatusRecord {
  uint8_t version = kStatusVersion;
  uint8_t flags = 0;
  uint16_t seq = 0;
  uint8_t battAge = kAgeNever;
  uint8_t socPct = 255;
  uint16_t battCentiVolts = 0;
  int16_t battDeciAmps = 0;
  uint16_t battMinutes = 65535;
  uint8_t chgAge = kAgeNever;
  uint16_t solarWatts = 0;
  uint16_t truckWatts = 0;
  int16_t chgDeciAmps = 0;
  uint8_t invAge = kAgeNever;
  uint8_t invBits = 0;
  uint16_t invWatts = 0;
  uint8_t circAge = kAgeNever;
  uint8_t circOn = 0;
  uint8_t circConfirmed = 0;
  uint8_t circReady = 0;
  uint8_t fanAge = kAgeNever;
  uint8_t fanMode = 0;
  uint8_t fanPct = 0;
  int16_t boxDeciC = 0;
  uint16_t cutCountdownS = 0;
};

bool operator==(const StatusRecord& a, const StatusRecord& b);

enum CommandOp : uint8_t {
  OP_SET_CIRCUIT = 1, OP_SET_INVERTER = 2, OP_FAN_MODE = 3, OP_FAN_PCT = 4,
  OP_RELEASE_BATTERY = 5, OP_KEEP_FRIDGE = 6, OP_CUT_NOW = 7
};

struct Command {
  uint8_t op = 0;
  uint8_t a = 0;
  uint8_t b = 0;
};

// Writes the 36-byte record; returns kStatusLen.
size_t encodeStatus(const StatusRecord& r, uint8_t out[kStatusLen]);
// False (and `out` untouched) for a wrong length or an unknown version.
bool decodeStatus(const uint8_t* data, size_t len, StatusRecord& out);
// False for an unknown op, a wrong length or a value out of range (see shared/protocol.md).
bool parseCommand(const uint8_t* data, size_t len, Command& out);
