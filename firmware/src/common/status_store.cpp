#include "status_store.h"

namespace {
void age(uint8_t& a) {
  if (a < 254) a++;  // 254 = "very old"; 255 = never read, left alone
}
}  // namespace

void StatusStore::apply(const Command& c) {
  switch (c.op) {
    case OP_SET_CIRCUIT: {
      const uint8_t bit = static_cast<uint8_t>(1u << (c.a - 1));
      if (c.b) r_.circOn |= bit;
      else r_.circOn &= static_cast<uint8_t>(~bit);
      r_.circConfirmed |= bit;  // slice 1: no Auxbeam yet, the store's own state is the truth
      break;
    }
    case OP_SET_INVERTER:
      if (c.a) r_.invBits |= INV_ON;
      else r_.invBits &= static_cast<uint8_t>(~INV_ON);
      break;
    case OP_FAN_MODE:
      r_.fanMode = c.a;
      break;
    case OP_FAN_PCT:
      r_.fanPct = c.a;
      break;
    default:  // release battery, keep fridge, cut now: later slices
      break;
  }
}

StatusRecord StatusStore::snapshot() {
  r_.seq = static_cast<uint16_t>(r_.seq + 1);
  return r_;
}

void StatusStore::tickSecond() {
  age(r_.battAge);
  age(r_.chgAge);
  age(r_.invAge);
  age(r_.circAge);
  age(r_.fanAge);
}
