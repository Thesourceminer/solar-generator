#include "espnow_link.h"

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

namespace {
const uint8_t kBroadcast[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
const uint8_t kChannel = 1;
EspNowFrameHandler handler = nullptr;

void onRecv(const uint8_t* /*mac*/, const uint8_t* data, int len) {
  if (handler && data && len > 0) handler(data, static_cast<size_t>(len));
}
}  // namespace

bool espnowBegin(EspNowFrameHandler onFrame) {
  handler = onFrame;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(kChannel, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_recv_cb(onRecv);
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, kBroadcast, 6);
  peer.channel = kChannel;
  peer.encrypt = false;
  if (!esp_now_is_peer_exist(kBroadcast) && esp_now_add_peer(&peer) != ESP_OK) return false;
  return true;
}

bool espnowSend(const uint8_t* data, size_t len) {
  if (!data || len == 0 || len > ESP_NOW_MAX_DATA_LEN) return false;
  return esp_now_send(kBroadcast, data, len) == ESP_OK;
}
