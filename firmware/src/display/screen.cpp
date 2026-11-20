#include "screen.h"

#include <Arduino.h>
#include <Wire.h>
#include <lvgl.h>

#include "backlight.h"
#include "crowpanel43.h"
#include "stale.h"

namespace {

CrowPanel43 gfx;
CommandSink commandSink = nullptr;

// Board revision, found at boot (ELECROW's README):
//   V1.1-V1.3 ("latest"): STC8H1K28 controller at 0x30: backlight 0 (brightest) .. 245 (off); 250 = enable touch.
//   V1.0 ("old"):         TCA9534 expander at 0x18: IO1 = backlight on/off, IO2 = touch reset.
enum class Board { Unknown, V10, V11 };
Board board = Board::Unknown;
const uint8_t kStc8Addr = 0x30;
const uint8_t kTouchAddr = 0x5D;

// Backlight: full for 60 s after a touch, dim until 5 min, then off (V1.0 can only do on/off).
uint32_t lastTouchMs = 0;
BacklightLevel backlight = BACKLIGHT_FULL;
bool swallowTouch = false;

const char* kCircuitNames[6] = {"Fridge", "LED", "USB charging", "12 V plug", "12 V plug", "Pump / Starlink"};

// Colours from the approved app mockup
const lv_color_t kBg = lv_color_hex(0x0f1115), kCard = lv_color_hex(0x1b1e24), kText = lv_color_hex(0xf1efe8),
                 kMuted = lv_color_hex(0x888780), kGood = lv_color_hex(0x5dcaa5), kSun = lv_color_hex(0xef9f27),
                 kDanger = lv_color_hex(0xe24b4a);

// ---- board controller writes ----
// Before gfx.init() the Arduino Wire driver owns the I2C pins (ELECROW's startup); after it, LovyanGFX owns port 0
// for the touch chip, so later writes go through LovyanGFX's own I2C functions on the same port.
bool lgfxOwnsI2c = false;

bool i2cWrite(uint8_t addr, const uint8_t* bytes, uint8_t n) {
  if (lgfxOwnsI2c) return lgfx::i2c::transactionWrite(I2C_NUM_0, addr, bytes, n).has_value();
  Wire.beginTransmission(addr);
  Wire.write(bytes, n);
  return Wire.endTransmission() == 0;
}
bool i2cPresent(uint8_t addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}
void stc8Send(uint8_t value) { i2cWrite(kStc8Addr, &value, 1); }

uint8_t ioexOut = 0;
void ioexPin(uint8_t pin, bool high) {
  ioexOut = high ? (ioexOut | (1 << pin)) : (ioexOut & ~(1 << pin));
  const uint8_t b[] = {0x01, ioexOut};  // output port register
  i2cWrite(kIoExpanderAddr, b, 2);
}

void setBacklight(BacklightLevel level) {
  backlight = level;
  if (board == Board::V11) {
    stc8Send(level == BACKLIGHT_FULL ? stc8Brightness(100) : level == BACKLIGHT_DIM ? stc8Brightness(kDimPercent) : 245);
  } else if (board == Board::V10) {
    ioexPin(1, level != BACKLIGHT_OFF);
  }
}

// ELECROW's startup for each revision. Returns the revision found.
Board startBoard() {
  Wire.begin(kI2cSda, kI2cScl);
  delay(50);
  for (int attempt = 0; attempt < 10; attempt++) {  // V1.1+: wake the controller and the touch chip
    if (i2cPresent(kStc8Addr) && i2cPresent(kTouchAddr)) {
      stc8Send(stc8Brightness(100));
      return Board::V11;
    }
    if (i2cPresent(kIoExpanderAddr) && !i2cPresent(kStc8Addr)) break;  // an old V1.0 board
    stc8Send(250);  // 250 = activate touch
    pinMode(1, OUTPUT);
    digitalWrite(1, LOW);
    delay(120);
    pinMode(1, INPUT);
    delay(100);
  }
  if (i2cPresent(kIoExpanderAddr)) {
    const uint8_t cfg[] = {0x03, 0b11100001};  // IO1-IO4 outputs
    i2cWrite(kIoExpanderAddr, cfg, 2);
    ioexPin(1, true);  // backlight on
    pinMode(1, OUTPUT);
    digitalWrite(1, LOW);
    ioexPin(2, false);  // touch reset
    delay(20);
    ioexPin(2, true);
    delay(100);
    pinMode(1, INPUT);
    return Board::V10;
  }
  return Board::Unknown;
}

// ---- LVGL glue ----
lv_disp_draw_buf_t drawBuf;
lv_disp_drv_t dispDrv;
lv_indev_drv_t indevDrv;

void flush(lv_disp_drv_t* disp, const lv_area_t* a, lv_color_t* px) {
  if (gfx.getStartCount() > 0) gfx.endWrite();
  gfx.pushImageDMA(a->x1, a->y1, a->x2 - a->x1 + 1, a->y2 - a->y1 + 1, reinterpret_cast<lgfx::rgb565_t*>(&px->full));
  lv_disp_flush_ready(disp);
}

void readTouch(lv_indev_drv_t*, lv_indev_data_t* data) {
  uint16_t x, y;
  data->state = LV_INDEV_STATE_REL;
  if (!gfx.getTouch(&x, &y)) {
    swallowTouch = false;
    return;
  }
  lastTouchMs = millis();
  if (backlight != BACKLIGHT_FULL) {  // a touch on a dim or dark screen only wakes it
    setBacklight(BACKLIGHT_FULL);
    swallowTouch = true;
  }
  if (swallowTouch) return;
  data->state = LV_INDEV_STATE_PR;
  data->point.x = x;
  data->point.y = y;
}

// ---- widgets ----
lv_obj_t *demoBadge, *linkBanner;
lv_obj_t *battArc, *battPct, *battVolts, *battAmps, *battTime;
lv_obj_t *solarVal, *truckVal, *invVal, *invBadge, *circList, *boxVal, *fanVal;
lv_obj_t *invSwitch, *circSwitch[6], *fanMode, *fanSlider, *fanNote;
bool updating = false;  // true while screenUpdate sets widget states (not a user action)

lv_obj_t* card(lv_obj_t* parent, int x, int y, int w, int h, const char* caption) {
  lv_obj_t* c = lv_obj_create(parent);
  lv_obj_set_pos(c, x, y);
  lv_obj_set_size(c, w, h);
  lv_obj_set_style_bg_color(c, kCard, 0);
  lv_obj_set_style_border_width(c, 0, 0);
  lv_obj_set_style_radius(c, 16, 0);
  lv_obj_set_style_pad_all(c, 12, 0);
  lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
  if (caption) {
    lv_obj_t* l = lv_label_create(c);
    lv_label_set_text(l, caption);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(l, kMuted, 0);
  }
  return c;
}

lv_obj_t* label(lv_obj_t* parent, const lv_font_t* font, lv_color_t color, int x, int y) {
  lv_obj_t* l = lv_label_create(parent);
  lv_obj_set_style_text_font(l, font, 0);
  lv_obj_set_style_text_color(l, color, 0);
  lv_obj_set_pos(l, x, y);
  lv_label_set_text(l, "--");
  return l;
}

void send(const uint8_t* b, size_t n) {
  if (!updating && commandSink) commandSink(b, n);
}

void onInverter(lv_event_t* e) {
  const uint8_t b[] = {OP_SET_INVERTER, static_cast<uint8_t>(lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED))};
  send(b, sizeof b);
}
void onCircuit(lv_event_t* e) {
  const uint8_t n = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
  const uint8_t b[] = {OP_SET_CIRCUIT, n, static_cast<uint8_t>(lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED))};
  send(b, sizeof b);
}
void onFanMode(lv_event_t* e) {
  const uint8_t b[] = {OP_FAN_MODE, static_cast<uint8_t>(lv_dropdown_get_selected(lv_event_get_target(e)) == 1)};
  send(b, sizeof b);
}
void onFanSlider(lv_event_t* e) {
  const uint8_t b[] = {OP_FAN_PCT, static_cast<uint8_t>(lv_slider_get_value(lv_event_get_target(e)))};
  send(b, sizeof b);
}

void buildStatus(lv_obj_t* t) {
  lv_obj_t* title = label(t, &lv_font_montserrat_28, kText, 20, 14);
  lv_label_set_text(title, "Solar Generator");

  lv_obj_t* b = card(t, 20, 60, 370, 200, "BATTERY");
  battArc = lv_arc_create(b);
  lv_obj_set_size(battArc, 140, 140);
  lv_obj_set_pos(battArc, 0, 24);
  lv_arc_set_range(battArc, 0, 100);
  lv_arc_set_bg_angles(battArc, 0, 360);
  lv_arc_set_rotation(battArc, 270);
  lv_obj_remove_style(battArc, nullptr, LV_PART_KNOB);
  lv_obj_clear_flag(battArc, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_arc_color(battArc, kGood, LV_PART_INDICATOR);
  battPct = lv_label_create(battArc);
  lv_obj_set_style_text_font(battPct, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(battPct, kText, 0);
  lv_obj_center(battPct);
  battVolts = label(b, &lv_font_montserrat_40, kText, 160, 40);
  battAmps = label(b, &lv_font_montserrat_20, kGood, 160, 100);
  battTime = label(b, &lv_font_montserrat_14, kMuted, 160, 130);

  solarVal = label(card(t, 410, 60, 175, 95, "SOLAR"), &lv_font_montserrat_28, kText, 0, 28);
  truckVal = label(card(t, 605, 60, 175, 95, "TRUCK"), &lv_font_montserrat_28, kText, 0, 28);
  lv_obj_t* inv = card(t, 410, 165, 370, 95, "INVERTER");
  invVal = label(inv, &lv_font_montserrat_28, kText, 0, 28);
  invBadge = label(inv, &lv_font_montserrat_20, kGood, 250, 32);

  circList = label(card(t, 20, 270, 470, 150, "CIRCUITS ON"), &lv_font_montserrat_20, kText, 0, 28);
  lv_label_set_long_mode(circList, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(circList, 440);

  lv_obj_t* box = card(t, 510, 270, 270, 150, "BOX  /  FANS");
  boxVal = label(box, &lv_font_montserrat_40, kText, 0, 30);
  fanVal = label(box, &lv_font_montserrat_20, kMuted, 0, 90);
}

void buildControls(lv_obj_t* t) {
  lv_obj_t* title = label(t, &lv_font_montserrat_28, kText, 20, 14);
  lv_label_set_text(title, "Controls");

  lv_obj_t* inv = card(t, 20, 60, 370, 80, nullptr);
  lv_label_set_text(label(inv, &lv_font_montserrat_20, kText, 0, 14), "Inverter (120 V outlets)");
  invSwitch = lv_switch_create(inv);
  lv_obj_align(invSwitch, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_add_event_cb(invSwitch, onInverter, LV_EVENT_VALUE_CHANGED, nullptr);

  lv_obj_t* fans = card(t, 410, 60, 370, 150, "FANS");
  fanMode = lv_dropdown_create(fans);
  lv_dropdown_set_options(fanMode, "Auto\nManual");
  lv_obj_set_pos(fanMode, 0, 26);
  lv_obj_set_width(fanMode, 150);
  lv_obj_add_event_cb(fanMode, onFanMode, LV_EVENT_VALUE_CHANGED, nullptr);
  fanSlider = lv_slider_create(fans);
  lv_slider_set_range(fanSlider, 0, 100);
  lv_obj_set_pos(fanSlider, 170, 40);
  lv_obj_set_width(fanSlider, 170);
  lv_obj_add_event_cb(fanSlider, onFanSlider, LV_EVENT_RELEASED, nullptr);
  fanNote = label(fans, &lv_font_montserrat_14, kMuted, 0, 90);

  lv_obj_t* circ = card(t, 20, 150, 370, 300, "AUXBEAM CIRCUITS");
  for (int i = 0; i < 6; i++) {
    lv_obj_t* l = label(circ, &lv_font_montserrat_20, kText, 0, 30 + i * 42);
    lv_label_set_text_fmt(l, "%d  %s", i + 1, kCircuitNames[i]);
    circSwitch[i] = lv_switch_create(circ);
    lv_obj_set_pos(circSwitch[i], 270, 26 + i * 42);
    lv_obj_add_event_cb(circSwitch[i], onCircuit, LV_EVENT_VALUE_CHANGED, reinterpret_cast<void*>(static_cast<uintptr_t>(i + 1)));
  }
}

void setChecked(lv_obj_t* o, bool on) {
  if (on) lv_obj_add_state(o, LV_STATE_CHECKED);
  else lv_obj_clear_state(o, LV_STATE_CHECKED);
}

void setEnabled(lv_obj_t* o, bool on) {
  if (on) lv_obj_clear_state(o, LV_STATE_DISABLED);
  else lv_obj_add_state(o, LV_STATE_DISABLED);
}

}  // namespace

void screenBegin(CommandSink sink) {
  commandSink = sink;
  board = startBoard();
  Serial.printf("Display board: %s\n", board == Board::V11 ? "V1.1-V1.3" : board == Board::V10 ? "V1.0" : "unknown");
  {  // ELECROW's pixel clock: 16 MHz on V1.1-V1.3, 21 MHz on V1.0
    auto cfg = gfx.bus.config();
    cfg.freq_write = board == Board::V10 ? 21000000 : 16000000;
    gfx.bus.config(cfg);
  }
  Wire.end();  // hand the I2C pins to LovyanGFX (touch) from here on
  gfx.init();
  lgfxOwnsI2c = true;
  gfx.initDMA();
  gfx.startWrite();
  gfx.fillScreen(TFT_BLACK);

  lv_init();
  // Two full-screen buffers in PSRAM; if PSRAM is missing, fall back to 40-line buffers in internal RAM.
  size_t px = kLcdW * kLcdH;
  auto* b1 = static_cast<lv_color_t*>(heap_caps_malloc(px * sizeof(lv_color_t), MALLOC_CAP_SPIRAM));
  auto* b2 = static_cast<lv_color_t*>(heap_caps_malloc(px * sizeof(lv_color_t), MALLOC_CAP_SPIRAM));
  if (!b1 || !b2) {
    Serial.println("Display: no PSRAM for full-screen buffers, using small ones");
    px = kLcdW * 40;
    b1 = static_cast<lv_color_t*>(heap_caps_malloc(px * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    b2 = static_cast<lv_color_t*>(heap_caps_malloc(px * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  }
  lv_disp_draw_buf_init(&drawBuf, b1, b2, px);
  lv_disp_drv_init(&dispDrv);
  dispDrv.hor_res = kLcdW;
  dispDrv.ver_res = kLcdH;
  dispDrv.flush_cb = flush;
  dispDrv.draw_buf = &drawBuf;
  lv_disp_drv_register(&dispDrv);
  lv_indev_drv_init(&indevDrv);
  indevDrv.type = LV_INDEV_TYPE_POINTER;
  indevDrv.read_cb = readTouch;
  lv_indev_drv_register(&indevDrv);

  lv_obj_t* scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, kBg, 0);
  lv_obj_t* tv = lv_tileview_create(scr);
  lv_obj_set_style_bg_color(tv, kBg, 0);
  buildStatus(lv_tileview_add_tile(tv, 0, 0, LV_DIR_RIGHT));
  buildControls(lv_tileview_add_tile(tv, 1, 0, LV_DIR_LEFT));

  demoBadge = label(scr, &lv_font_montserrat_20, kSun, 700, 18);
  lv_label_set_text(demoBadge, "DEMO");
  lv_obj_add_flag(demoBadge, LV_OBJ_FLAG_HIDDEN);
  linkBanner = label(scr, &lv_font_montserrat_20, kDanger, 20, 440);
  lv_obj_set_width(linkBanner, kLcdW - 40);
  lv_obj_set_style_text_align(linkBanner, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_text(linkBanner, "No link to the box");
  lastTouchMs = millis();
}

void screenUpdate(const StatusRecord* r, uint32_t ms, bool versionProblem) {
  updating = true;
  const bool down = (r == nullptr) || linkDown(ms);
  if (versionProblem) {
    lv_label_set_text(linkBanner, "Update the box screen: the box sends a newer format");
    lv_obj_clear_flag(linkBanner, LV_OBJ_FLAG_HIDDEN);
  } else if (down) {
    lv_label_set_text(linkBanner, "No link to the box");
    lv_obj_clear_flag(linkBanner, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(linkBanner, LV_OBJ_FLAG_HIDDEN);
  }
  StatusRecord none;
  const StatusRecord& s = r ? *r : none;
  if (s.flags & FLAG_DEMO) lv_obj_clear_flag(demoBadge, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_add_flag(demoBadge, LV_OBJ_FLAG_HIDDEN);

  const bool battStale = r == nullptr || groupStale(s.battAge, ms) || s.socPct > 100;
  lv_arc_set_value(battArc, battStale ? 0 : s.socPct);
  if (battStale) {
    lv_label_set_text(battPct, "--");
    lv_label_set_text(battVolts, "--");
    lv_label_set_text(battAmps, "");
    lv_label_set_text(battTime, "");
  } else {
    lv_label_set_text_fmt(battPct, "%u%%", s.socPct);
    lv_label_set_text_fmt(battVolts, "%u.%02u V", s.battCentiVolts / 100, s.battCentiVolts % 100);
    const int a = s.battDeciAmps < 0 ? -s.battDeciAmps : s.battDeciAmps;
    lv_label_set_text_fmt(battAmps, "%s %d.%d A", s.battDeciAmps >= 0 ? "charging" : "using", a / 10, a % 10);
    lv_obj_set_style_text_color(battAmps, s.battDeciAmps >= 0 ? kGood : kSun, 0);
    if (s.battMinutes == 65535) lv_label_set_text(battTime, "");
    else if (s.battMinutes < 60) lv_label_set_text_fmt(battTime, "%s in about %u min", s.battDeciAmps >= 0 ? "full" : "empty", s.battMinutes);
    else lv_label_set_text_fmt(battTime, "%s in about %u h", s.battDeciAmps >= 0 ? "full" : "empty", (s.battMinutes + 30) / 60);
  }

  const bool chgStale = r == nullptr || groupStale(s.chgAge, ms);
  if (chgStale) {
    lv_label_set_text(solarVal, "--");
    lv_label_set_text(truckVal, "--");
  } else {
    lv_label_set_text_fmt(solarVal, "%u W", s.solarWatts);
    lv_label_set_text_fmt(truckVal, "%u W", s.truckWatts);
  }

  const bool invStale = r == nullptr || groupStale(s.invAge, ms);
  const bool invOn = s.invBits & INV_ON;
  if (invStale) {
    lv_label_set_text(invVal, "--");
    lv_label_set_text(invBadge, "");
  } else {
    lv_label_set_text_fmt(invVal, "%u W out", invOn ? s.invWatts : 0);
    lv_label_set_text(invBadge, (s.invBits & INV_ERR) ? "ERR" : (invOn ? "On" : "Off"));
    lv_obj_set_style_text_color(invBadge, (s.invBits & INV_ERR) ? kDanger : (invOn ? kGood : kMuted), 0);
  }
  setChecked(invSwitch, invOn);
  setEnabled(invSwitch, !invStale);

  const bool circStale = r == nullptr || groupStale(s.circAge, ms);
  if (circStale) {
    lv_label_set_text(circList, "--");
  } else {
    char buf[160] = "";
    for (int i = 0; i < 6; i++) {
      if (!(s.circOn & (1 << i))) continue;
      char item[32];
      snprintf(item, sizeof item, "%s%d %s%s", buf[0] ? "   " : "", i + 1, kCircuitNames[i],
               (s.circConfirmed & (1 << i)) ? "" : " (sent)");
      strncat(buf, item, sizeof buf - strlen(buf) - 1);
    }
    lv_label_set_text(circList, buf[0] ? buf : "None");
  }
  for (int i = 0; i < 6; i++) {
    setChecked(circSwitch[i], s.circOn & (1 << i));
    setEnabled(circSwitch[i], s.circReady && !circStale);
  }

  const bool fanStale = r == nullptr || groupStale(s.fanAge, ms);
  if (fanStale) {
    lv_label_set_text(boxVal, "--");
    lv_label_set_text(fanVal, "--");
  } else {
    const int f10 = s.boxDeciC * 9 / 5 + 320;  // tenths of a degree F
    lv_label_set_text_fmt(boxVal, "%d F", (f10 >= 0 ? f10 + 5 : f10 - 5) / 10);
    if (s.fanPct == 0) lv_label_set_text(fanVal, "Fans off");
    else lv_label_set_text_fmt(fanVal, "Fans %s%u%%", s.fanMode == 0 ? "auto " : "", s.fanPct);
  }
  lv_dropdown_set_selected(fanMode, s.fanMode == 1 ? 1 : 0);
  if (!lv_obj_has_state(fanSlider, LV_STATE_PRESSED)) lv_slider_set_value(fanSlider, s.fanPct, LV_ANIM_OFF);
  setEnabled(fanMode, !fanStale);
  setEnabled(fanSlider, !fanStale && s.fanMode == 1);
  lv_label_set_text(fanNote, fanStale ? "No recent reading" : s.fanMode == 1 ? "Manual speed" : "Auto: speed follows box temperature");
  updating = false;
}

void screenLoop() {
  lv_timer_handler();
  const BacklightLevel want = backlightFor(millis() - lastTouchMs);
  if (want != backlight && want != BACKLIGHT_FULL) setBacklight(want);  // only a touch brings it back to full
}
