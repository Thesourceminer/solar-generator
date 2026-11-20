# Solar Generator link protocol (v1)

One binary **status record** and a small **command** set, used on two links:

- **Phone <-> brain** over Bluetooth LE (GATT, encrypted and bonded).
- **Brain <-> display** over ESP-NOW (the same bytes).

Source of truth for the firmware (`firmware/common/protocol.*`) and the Off-Grid HQ app.
Test vectors: `vectors.json` (both test suites run them). After editing `vectors.json` run
`python -I shared/gen_vectors.py` to refresh the hex and `firmware/test/test_protocol/vectors_gen.h`.

## BLE UUIDs

| What | UUID | Properties |
|---|---|---|
| Service | `220eacc7-b23c-4bd3-92d5-a96add0ac036` | |
| Status | `9dc21368-9a30-410e-8878-adef2369a0f5` | read + notify (about 1 Hz) |
| Command | `b8544811-1d1a-45f5-ada9-14ede8cbbae1` | write with response |

## Status record v1: 36 bytes, little-endian

| Off | Type | Field | Notes |
|---|---|---|---|
| 0 | u8 | version | = 1 |
| 1 | u8 | flags | bit0 demo, bit1 pairing window open, bit2 low-battery warning, bit3 cut pending |
| 2 | u16 | seq | +1 per record, wraps 65535 -> 0 |
| 4 | u8 | battAge | seconds since the last battery reading, 255 = never |
| 5 | u8 | socPct | 255 = unknown |
| 6 | u16 | battCentiVolts | 1328 = 13.28 V |
| 8 | i16 | battDeciAmps | + charging, - using |
| 10 | u16 | battMinutes | to full when charging, to empty when using; 65535 = unknown |
| 12 | u8 | chgAge | |
| 13 | u8 | reserved | 0 |
| 14 | u16 | solarWatts | |
| 16 | u16 | truckWatts | |
| 18 | i16 | chgDeciAmps | |
| 20 | u8 | invAge | |
| 21 | u8 | invBits | bit0 on, bit1 ERR light |
| 22 | u16 | invWatts | |
| 24 | u8 | circAge | |
| 25 | u8 | circOn | bit n-1 = circuit n on |
| 26 | u8 | circConfirmed | bit set = confirmed by the Auxbeam, clear = last sent by the board |
| 27 | u8 | circReady | 0 = Auxbeam not set up yet |
| 28 | u8 | fanAge | |
| 29 | u8 | fanMode | 0 auto, 1 manual |
| 30 | u8 | fanPct | 0-100 |
| 31 | u8 | reserved | 0 |
| 32 | i16 | boxDeciC | 312 = 31.2 C |
| 34 | u16 | cutCountdownS | 0 = none |

Any record whose length is not 36, or whose version is not 1, is ignored by the receiver.
A group whose age is 255, or older than 10 s, is shown as "--".

## Commands (write)

| Bytes | Meaning |
|---|---|
| `01 n v` | set circuit n (1-6) on (v=1) / off (v=0) |
| `02 v` | inverter on / off |
| `03 m` | fan mode (0 auto, 1 manual) |
| `04 p` | fan percent (0-100) |
| `05 m` | release the battery Bluetooth link for m minutes (1-60) |
| `06` | keep the fridge on during a low-battery cut |
| `07` | cut now |

Anything else (unknown op, wrong length, value out of range) is rejected.
