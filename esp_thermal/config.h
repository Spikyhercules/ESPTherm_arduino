#pragma once

// ---------------- MLX90640 (I2C) ----------------
// Avoid GPIO 26-32 (flash) and 35-37 (octal PSRAM) on N8R8/N16R8 modules.
#define PIN_I2C_SDA     4
#define PIN_I2C_SCL     5
#define I2C_CLOCK_HZ    400000      // 400000 if wiring is long/unstable, up to 1000000
#define DEFAULT_RATE_HZ 4           // sensor subpage rate (1,2,4,8). Full frames/s = rate/2

// ---------------- Local display (LVGL) ----------------
// TFT pins/driver are set in platformio.ini (TFT_eSPI build flags).
#define UI_ROTATION     1           // 1 = landscape 320x240, 3 = landscape flipped
#define UI_INVERT       true        // panel colour inversion (true if colours look like a negative)
#define UI_SMOOTH       0.5f        // temporal smoothing of the *displayed image only* (0..1, 1 = off)
#define UI_MIN_SPAN     5.0f        // minimum colour-scale span in degC (stops noise being amplified)

// ---------------- Network ----------------
#define HOSTNAME        "thermalcam"          // also http://thermalcam.local
#define AP_SSID         "ThermalCam-Setup"
#define AP_PASS         "thermalcam"          // min 8 chars
#define WIFI_TIMEOUT_MS 15000

#define USE_STATIC_IP   0
#if USE_STATIC_IP
#define STATIC_IP       192,168,10,60
#define STATIC_GW       192,168,10,1
#define STATIC_MASK     255,255,255,0
#define STATIC_DNS      192,168,10,1
#endif

// ---------------- Web OTA (http://thermalcam.local/update) ----------------
#define OTA_USER        "admin"
#define OTA_PASS        ""            // set a password to require basic-auth on /update ("" = open)
