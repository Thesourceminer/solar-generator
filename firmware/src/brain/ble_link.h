// Phone link: one GATT service (UUIDs in shared/protocol.md). Status = read + notify, command = write; both need an
// encrypted, bonded, MITM-protected link (6-digit passkey, printed to Serial in slice 1; on the screen in slice 2).
#pragma once
#include "protocol.h"
#include "status_store.h"

void bleBegin();
void bleNotify(const StatusRecord& r);
// Commands written by the phone are parsed in the BLE task and queued; call from loop() to apply them.
void bleApplyPending(StatusStore& store);
