// ELECROW CrowPanel Advance 4.3 in (ESP32-S3, 800 x 480 RGB panel, GT911 touch).
// Pin numbers and panel timings are taken from ELECROW's own example
// (github.com/Elecrow-RD/CrowPanel-Advance-4.3-HMI-ESP32-S3-AI-Powered-IPS-Touch-Screen-800x480,
//  "Squareline_studio Instructions for use/code/BigInch_LVGL/LovyanGFX_Driver.h" and BigInch_LVGL.ino).
#pragma once
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <driver/i2c.h>

static const int kLcdW = 800;
static const int kLcdH = 480;
// Board I2C (touch + TCA9534 I/O expander at 0x18: IO1 = backlight, IO2 = touch reset)
static const int kI2cSda = 15;
static const int kI2cScl = 16;
static const uint8_t kIoExpanderAddr = 0x18;

class CrowPanel43 : public lgfx::LGFX_Device {
 public:
  lgfx::Bus_RGB bus;
  lgfx::Panel_RGB panel;
  lgfx::Touch_GT911 touch;

  CrowPanel43() {
    {
      auto cfg = panel.config();
      cfg.memory_width = kLcdW;
      cfg.memory_height = kLcdH;
      cfg.panel_width = kLcdW;
      cfg.panel_height = kLcdH;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      panel.config(cfg);
    }
    {
      auto cfg = panel.config_detail();
      cfg.use_psram = 1;
      panel.config_detail(cfg);
    }
    {
      auto cfg = bus.config();
      cfg.panel = &panel;
      cfg.pin_d0 = GPIO_NUM_21;   // B0
      cfg.pin_d1 = GPIO_NUM_47;   // B1
      cfg.pin_d2 = GPIO_NUM_48;   // B2
      cfg.pin_d3 = GPIO_NUM_45;   // B3
      cfg.pin_d4 = GPIO_NUM_38;   // B4
      cfg.pin_d5 = GPIO_NUM_9;    // G0
      cfg.pin_d6 = GPIO_NUM_10;   // G1
      cfg.pin_d7 = GPIO_NUM_11;   // G2
      cfg.pin_d8 = GPIO_NUM_12;   // G3
      cfg.pin_d9 = GPIO_NUM_13;   // G4
      cfg.pin_d10 = GPIO_NUM_14;  // G5
      cfg.pin_d11 = GPIO_NUM_7;   // R0
      cfg.pin_d12 = GPIO_NUM_17;  // R1
      cfg.pin_d13 = GPIO_NUM_18;  // R2
      cfg.pin_d14 = GPIO_NUM_3;   // R3
      cfg.pin_d15 = GPIO_NUM_46;  // R4
      cfg.pin_henable = GPIO_NUM_42;
      cfg.pin_vsync = GPIO_NUM_41;
      cfg.pin_hsync = GPIO_NUM_40;
      cfg.pin_pclk = GPIO_NUM_39;
      cfg.freq_write = 21000000;
      cfg.hsync_polarity = 0;
      cfg.hsync_front_porch = 8;
      cfg.hsync_pulse_width = 4;
      cfg.hsync_back_porch = 8;
      cfg.vsync_polarity = 0;
      cfg.vsync_front_porch = 8;
      cfg.vsync_pulse_width = 4;
      cfg.vsync_back_porch = 8;
      cfg.pclk_idle_high = 1;
      bus.config(cfg);
    }
    panel.setBus(&bus);
    {
      auto cfg = touch.config();
      cfg.x_min = 0;
      cfg.x_max = kLcdW;
      cfg.y_min = 0;
      cfg.y_max = kLcdH;
      cfg.pin_int = -1;
      cfg.bus_shared = false;
      cfg.offset_rotation = 0;
      cfg.i2c_port = I2C_NUM_0;
      cfg.pin_sda = GPIO_NUM_15;
      cfg.pin_scl = GPIO_NUM_16;
      cfg.pin_rst = -1;
      cfg.freq = 400000;
      cfg.i2c_addr = 0x5D;
      touch.config(cfg);
      panel.setTouch(&touch);
    }
    setPanel(&panel);
  }
};
