# ESP32-S3 MLX90640 Thermal Camera

A 32×24 thermal camera built on an ESP32-S3 and an MLX90640 sensor. One sensor and temperature engine feeds two displays at the same time:

- a local **ILI9341 320×240 LVGL screen**
- a **live web page** over WebSocket, for a phone, tablet or PC

Interpolation, smoothing and colour palettes are display-only. Min, max, average, centre, hotspot and coldspot are all computed once from the 768 real pixel values and shared by both screens, so the numbers always match.

## Features

- Live thermal image on the LCD with a colour scale bar, hot and cold spot markers and a centre reticle
- Web UI: smooth interpolated image, crosshair, clickable spot probes, fixed or auto temperature scale, freeze, PNG snapshot and CSV export
- Ironbow and Inferno palettes, switchable from the LCD side or the web page
- Selectable sensor rate (1, 2, 4 or 8 Hz subpage rate)
- Wi-Fi with **AP fallback**: if it can't join your network it starts its own access point, and the LCD shows a full-screen setup page with the SSID, password and URL
- Wi-Fi credentials set from the browser (`/setup`) and saved to flash
- **Web OTA**: upload new firmware from the browser at `/update`, with progress on the LCD
- mDNS: reachable at `http://thermalcam.local`
- The web page is embedded in the firmware, so there is no filesystem to upload

## Hardware

- ESP32-S3 dev board (tested with an N8R8 module: 8 MB flash, 8 MB octal PSRAM)
- MLX90640 breakout (I²C, 32×24 pixels)
- ILI9341 TFT (320×240, SPI)

### Default wiring

MLX90640:

- SDA to GPIO 4
- SCL to GPIO 5
- VIN to 3V3
- GND to GND

ILI9341:

- MOSI to GPIO 11
- SCLK to GPIO 12
- MISO to GPIO 13
- CS to GPIO 10
- DC to GPIO 8
- RST to GPIO 9
- Backlight to 3V3 (not GPIO-controlled)

Avoid GPIO 26–32 (flash) and 35–37 (octal PSRAM) on N8R8 and N16R8 modules. The I²C pins are set in `config.h`, and the TFT pins in `build_opt.h`.

## Building with Arduino IDE

Tested with **Arduino IDE 1.8.19** and **ESP32 Arduino core 3.3.12**.

### 1. Libraries

- lvgl 8.3.x (**not** 9.x)
- TFT_eSPI 2.5.43
- Adafruit MLX90640
- Adafruit BusIO
- ArduinoJson 7.x
- AsyncTCP, mathieucarbou fork (tested with 3.4.10), install from GitHub ZIP
- ESPAsyncWebServer, mathieucarbou fork, install from GitHub ZIP

### 2. LVGL config

Copy `lv_conf.h` to `<Arduino>/libraries/lvgl/src/lv_conf.h`.

It needs at least:

```c
#define LV_COLOR_DEPTH     16
#define LV_COLOR_16_SWAP   0
#define LV_USE_CANVAS      1
#define LV_USE_IMG         1
#define LV_USE_LABEL       1
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_20 1
```

If you already have an `lv_conf.h` from another project, edit that one instead of replacing it. Make sure there is only one `lv_conf.h`. The sketch feeds LVGL its own tick when `LV_TICK_CUSTOM` is not set, so either style works.

### 3. Sketch folder

Keep all files together in a folder named `esp_thermal`, including **`build_opt.h`**. It carries the compiler flags for the TFT pins, the async web server and LVGL. TFT_eSPI is configured entirely from these flags (`USER_SETUP_LOADED=1`), so you don't edit its `User_Setup.h`.

### 4. Board settings

- Board: ESP32S3 Dev Module
- PSRAM: OPI PSRAM
- Flash size: 8 MB
- Flash mode: QIO 80 MHz
- Partition scheme: **8M with spiffs (3MB APP)**, an OTA scheme with two app slots (not Huge APP)
- Core Debug Level: None

### 5. Flash

Edit `secrets.h` with your Wi-Fi details (or skip this and use the AP fallback), then upload over USB. Open the serial monitor at 115200 baud to see the IP. Browse to `http://<ip>` or `http://thermalcam.local`.

## First-time Wi-Fi setup

If the board can't join a network, the LCD shows a **Wi-Fi Setup** page:

1. Join the Wi-Fi network `ThermalCam-Setup` (password `thermalcam`)
2. Open `http://192.168.4.1/setup` and enter your network details

The credentials are saved to flash, the board reconnects, and the LCD page closes. Once connected, the IP appears at the bottom of the stats column on the LCD. The AP name and password can be changed in `config.h`.

## Firmware updates over Wi-Fi

Open `http://thermalcam.local/update` (or `http://<ip>/update`, or the link on `/setup`).

In Arduino IDE use **Sketch → Export compiled Binary** and upload the file ending **`.ino.bin`**. Don't use the bootloader, partitions or merged files. Progress shows in the browser and on the LCD, and the board restarts when finished.

To require a login, set `OTA_PASS` in `config.h`. The user name is `OTA_USER` (default `admin`).

## Configuration (`config.h`)

- `PIN_I2C_SDA` / `PIN_I2C_SCL` (4 / 5): MLX90640 pins
- `I2C_CLOCK_HZ` (400000): drop it if reads fail on long wiring, raise it for speed
- `DEFAULT_RATE_HZ` (4): sensor subpage rate; full frames per second = rate ÷ 2
- `UI_ROTATION` (1): 1 or 3 for landscape / flipped
- `UI_INVERT` (true): panel colour inversion
- `UI_SMOOTH` (0.5): temporal smoothing of the displayed image only
- `UI_MIN_SPAN` (5.0): minimum colour-scale span in °C, so noise isn't amplified
- `HOSTNAME` (thermalcam): mDNS name
- `AP_SSID` / `AP_PASS` (ThermalCam-Setup / thermalcam): fallback access point, password minimum 8 characters
- `USE_STATIC_IP` (0): set to 1 and fill in the addresses for a fixed IP
- `OTA_USER` / `OTA_PASS` (admin / none): basic-auth for `/update`

## Project files

- `esp_thermal.ino`: setup and main loop
- `sensor.cpp`: MLX90640 task on core 0, bad-pixel cleanup, temperature engine
- `palette.cpp`: Ironbow and Inferno 256-entry RGB565 lookup tables
- `display_ui.cpp`: LVGL screen, bilinear 7× image, colour bar, stats, Wi-Fi setup page
- `web.cpp`: Wi-Fi (STA plus AP fallback), async web server, WebSocket, `/setup`, `/update`, `/api/status`
- `index_html.h`: web UI embedded in flash
- `config.h`: pins, rates, display and network settings
- `secrets.h`: default Wi-Fi credentials
- `build_opt.h`: compiler flags (TFT pins, async server, LVGL)

## WebSocket protocol

Binary frame, little-endian, 1564 bytes:

```
u8 type=1 | u8 flags | u16 seq | f32 min,max,avg,centre | u16 hotIdx,coldIdx | u32 ms | int16[768] centi-°C
```

Browser to device, as text commands:

```json
{"cmd":"rate","hz":4}
{"cmd":"palette","name":"inferno"}
```

Pixel coordinates are (x, y) from the top-left, x 0–31 and y 0–23. The centre is pixel (16, 12).

## Troubleshooting

**Boot loop with `StoreProhibited` right after the PSRAM line**
Add `-DUSE_HSPI_PORT` to `build_opt.h`. This is a known TFT_eSPI issue on the ESP32-S3.

**`lv_canvas_create was not declared`**
LVGL is reading an `lv_conf.h` with `LV_USE_CANVAS` off. Fix the one in `libraries/lvgl/src/` and remove duplicates.

**LCD draws once, then labels stay on `--` and `NO DATA`**
LVGL has no tick source. The sketch now handles this itself, so make sure you have the latest version.

**Colours look like a negative**
Set `UI_INVERT` to `true` (or `false` if they're inverted the other way).

**Wrong colour order (reds and blues swapped)**
Add `-DTFT_RGB_ORDER=TFT_BGR` to `build_opt.h`.

**Image rotated or flipped**
Change `UI_ROTATION` to 3.

**I²C read failures**
Lower `I2C_CLOCK_HZ` to 400000 and check the SDA and SCL wiring.

**OTA upload fails**
Check the partition scheme has two app slots. Huge APP doesn't support OTA.

## Notes

- The sensor rate is the subpage rate, so 4 Hz gives roughly 2 full frames per second. Try 8 Hz once the wiring is proven stable.
- Temperatures are raw MLX90640 values with a fixed emissivity assumption. Shiny metal surfaces read low.
