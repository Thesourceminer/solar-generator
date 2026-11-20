#include "ble_link.h"

#include <Arduino.h>
#include <NimBLEDevice.h>

namespace {
const char* kServiceUuid = "220eacc7-b23c-4bd3-92d5-a96add0ac036";
const char* kStatusUuid = "9dc21368-9a30-410e-8878-adef2369a0f5";
const char* kCommandUuid = "b8544811-1d1a-45f5-ada9-14ede8cbbae1";

NimBLECharacteristic* statusChr = nullptr;
QueueHandle_t commandQueue = nullptr;

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* server) override {
    Serial.println("BLE: phone connected");
    NimBLEDevice::startAdvertising();  // keep advertising so a second bonded phone can connect
  }
  void onDisconnect(NimBLEServer*) override {
    Serial.println("BLE: phone disconnected");
    NimBLEDevice::startAdvertising();
  }
  uint32_t onPassKeyRequest() override {
    const uint32_t key = esp_random() % 1000000;
    Serial.printf("BLE: pairing passkey %06u\n", key);
    return key;
  }
  void onAuthenticationComplete(ble_gap_conn_desc* desc) override {
    // Notifications only check "encrypted" in NimBLE 1.4, so a Just Works link could still read status:
    // anything short of an authenticated (passkey) bond is dropped.
    const bool ok = desc->sec_state.encrypted && desc->sec_state.authenticated && desc->sec_state.bonded;
    Serial.printf("BLE: pairing %s\n", ok ? "OK (passkey bond)" : "refused (not a passkey bond)");
    if (!ok) NimBLEDevice::getServer()->disconnect(desc->conn_handle);
  }
};

class CommandCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* chr) override {
    const std::string v = chr->getValue();
    Command c;
    if (!parseCommand(reinterpret_cast<const uint8_t*>(v.data()), v.size(), c)) {
      Serial.printf("BLE: rejected command (%u bytes)\n", static_cast<unsigned>(v.size()));
      return;
    }
    xQueueSend(commandQueue, &c, 0);
  }
};
}  // namespace

void bleBegin() {
  commandQueue = xQueueCreate(8, sizeof(Command));
  NimBLEDevice::init("Solar Generator");
  NimBLEDevice::setSecurityAuth(true, true, true);  // bonding, MITM, secure connections
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);

  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());
  NimBLEService* svc = server->createService(kServiceUuid);
  statusChr = svc->createCharacteristic(
      kStatusUuid, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::READ_AUTHEN);
  NimBLECharacteristic* cmd = svc->createCharacteristic(
      kCommandUuid, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::WRITE_AUTHEN);
  cmd->setCallbacks(new CommandCallbacks());
  svc->start();

  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(kServiceUuid);
  adv->setScanResponse(true);
  adv->start();
  Serial.println("BLE: advertising as \"Solar Generator\"");
}

void bleNotify(const StatusRecord& r) {
  if (!statusChr) return;
  uint8_t bytes[kStatusLen];
  encodeStatus(r, bytes);
  statusChr->setValue(bytes, kStatusLen);
  statusChr->notify();
}

void bleApplyPending(StatusStore& store) {
  if (!commandQueue) return;
  Command c;
  while (xQueueReceive(commandQueue, &c, 0) == pdTRUE) store.apply(c);
}
