#include "protocol.h"

namespace {

struct Writer {
  uint8_t* p;
  void u8(uint8_t v) { *p++ = v; }
  void u16(uint16_t v) {
    *p++ = static_cast<uint8_t>(v & 0xff);
    *p++ = static_cast<uint8_t>(v >> 8);
  }
  void i16(int16_t v) { u16(static_cast<uint16_t>(v)); }
};

struct Reader {
  const uint8_t* p;
  uint8_t u8() { return *p++; }
  uint16_t u16() {
    const uint16_t v = static_cast<uint16_t>(p[0] | (p[1] << 8));
    p += 2;
    return v;
  }
  int16_t i16() { return static_cast<int16_t>(u16()); }
};

}  // namespace

bool operator==(const StatusRecord& a, const StatusRecord& b) {
  uint8_t ea[kStatusLen], eb[kStatusLen];
  encodeStatus(a, ea);
  encodeStatus(b, eb);
  for (size_t i = 0; i < kStatusLen; i++)
    if (ea[i] != eb[i]) return false;
  return true;
}

size_t encodeStatus(const StatusRecord& r, uint8_t out[kStatusLen]) {
  Writer w{out};
  w.u8(r.version);
  w.u8(r.flags);
  w.u16(r.seq);
  w.u8(r.battAge);
  w.u8(r.socPct);
  w.u16(r.battCentiVolts);
  w.i16(r.battDeciAmps);
  w.u16(r.battMinutes);
  w.u8(r.chgAge);
  w.u8(0);  // reserved
  w.u16(r.solarWatts);
  w.u16(r.truckWatts);
  w.i16(r.chgDeciAmps);
  w.u8(r.invAge);
  w.u8(r.invBits);
  w.u16(r.invWatts);
  w.u8(r.circAge);
  w.u8(r.circOn);
  w.u8(r.circConfirmed);
  w.u8(r.circReady);
  w.u8(r.fanAge);
  w.u8(r.fanMode);
  w.u8(r.fanPct);
  w.u8(0);  // reserved
  w.i16(r.boxDeciC);
  w.u16(r.cutCountdownS);
  return kStatusLen;
}

bool decodeStatus(const uint8_t* data, size_t len, StatusRecord& out) {
  if (data == nullptr || len != kStatusLen || data[0] != kStatusVersion) return false;
  Reader rd{data};
  StatusRecord r;
  r.version = rd.u8();
  r.flags = rd.u8();
  r.seq = rd.u16();
  r.battAge = rd.u8();
  r.socPct = rd.u8();
  r.battCentiVolts = rd.u16();
  r.battDeciAmps = rd.i16();
  r.battMinutes = rd.u16();
  r.chgAge = rd.u8();
  rd.u8();  // reserved
  r.solarWatts = rd.u16();
  r.truckWatts = rd.u16();
  r.chgDeciAmps = rd.i16();
  r.invAge = rd.u8();
  r.invBits = rd.u8();
  r.invWatts = rd.u16();
  r.circAge = rd.u8();
  r.circOn = rd.u8();
  r.circConfirmed = rd.u8();
  r.circReady = rd.u8();
  r.fanAge = rd.u8();
  r.fanMode = rd.u8();
  r.fanPct = rd.u8();
  rd.u8();  // reserved
  r.boxDeciC = rd.i16();
  r.cutCountdownS = rd.u16();
  out = r;
  return true;
}

bool parseCommand(const uint8_t* data, size_t len, Command& out) {
  if (data == nullptr || len == 0) return false;
  Command c;
  c.op = data[0];
  switch (c.op) {
    case OP_SET_CIRCUIT:
      if (len != 3 || data[1] < 1 || data[1] > 6 || data[2] > 1) return false;
      c.a = data[1];
      c.b = data[2];
      break;
    case OP_SET_INVERTER:
    case OP_FAN_MODE:
      if (len != 2 || data[1] > 1) return false;
      c.a = data[1];
      break;
    case OP_FAN_PCT:
      if (len != 2 || data[1] > 100) return false;
      c.a = data[1];
      break;
    case OP_RELEASE_BATTERY:
      if (len != 2 || data[1] < 1 || data[1] > 60) return false;
      c.a = data[1];
      break;
    case OP_KEEP_FRIDGE:
    case OP_CUT_NOW:
      if (len != 1) return false;
      break;
    default:
      return false;
  }
  out = c;
  return true;
}
