#include "demo_source.h"

namespace {
// Triangle wave 0..1000..0 over `periodMs`, so the demo drifts without floating point.
uint32_t tri(uint32_t ms, uint32_t periodMs) {
  const uint32_t t = ms % periodMs;
  const uint32_t half = periodMs / 2;
  return t < half ? t * 1000 / half : (periodMs - t) * 1000 / half;
}
}  // namespace

void DemoSource::step(StatusStore& store, uint32_t ms) {
  StatusRecord& r = store.raw();
  const uint32_t sun = tri(ms, 600000);   // 10 min "day"
  const uint32_t wob = tri(ms, 47000);    // small wobble
  r.flags |= FLAG_DEMO;

  r.battAge = 0;
  r.socPct = static_cast<uint8_t>(60 + sun * 35 / 1000);            // 60-95 %
  r.battCentiVolts = static_cast<uint16_t>(1300 + sun * 40 / 1000);  // 13.00-13.40 V
  const int32_t solar = static_cast<int32_t>(sun * 260 / 1000 + wob * 20 / 1000);  // 0-280 W
  const int32_t load = (r.invBits & INV_ON) ? 340 : 45;
  r.battDeciAmps = static_cast<int16_t>((solar - load) * 10 / 13);
  r.battMinutes = r.battDeciAmps == 0 ? 65535 : static_cast<uint16_t>(30 + wob * 200 / 1000);

  r.chgAge = 0;
  r.solarWatts = static_cast<uint16_t>(solar);
  r.truckWatts = 0;
  r.chgDeciAmps = static_cast<int16_t>(solar * 10 / 13);

  r.invAge = 0;
  r.invWatts = (r.invBits & INV_ON) ? static_cast<uint16_t>(300 + wob * 80 / 1000) : 0;

  r.circAge = 0;
  r.circReady = 1;

  r.fanAge = 0;
  r.boxDeciC = static_cast<int16_t>(250 + sun * 90 / 1000);  // 25.0-34.0 C
  if (r.fanMode == 0) {
    const int pct = r.boxDeciC < 300 ? 0 : (r.boxDeciC - 300) * 100 / 40 + 20;
    r.fanPct = static_cast<uint8_t>(pct > 100 ? 100 : pct);
  }
}
