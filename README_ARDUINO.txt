ESP32-S3 MLX90640 Thermal Camera - Arduino IDE 1.8 version
==========================================================

1. Open esp_thermal/esp_thermal.ino (folder name must stay esp_thermal).
   build_opt.h in the sketch folder carries all the -D flags (TFT_eSPI pins, async web server, LVGL).
   (If colours look wrong after flashing: orange showing blue = remove -DTFT_RGB_ORDER=TFT_BGR; still inverted = remove -DTFT_INVERSION_ON.)
   Do NOT define the TFT pins in TFT_eSPI's User_Setup.h - USER_SETUP_LOADED=1 bypasses it.

2. Copy _copy_to_libraries/lv_conf.h to  <Arduino>/libraries/lvgl/src/lv_conf.h
   (same method as ThermaVolt). Replace if one exists, or merge: needs LV_COLOR_DEPTH 16,
   LV_COLOR_16_SWAP 0, LV_MEM_CUSTOM 1, LV_TICK_CUSTOM 1 (millis), fonts montserrat 12/14/20,
   LV_USE_CANVAS 1, LV_USE_LABEL 1.

3. Libraries (Library Manager unless noted):
   - lvgl 8.3.x (NOT 9.x)
   - TFT_eSPI 2.5.43
   - Adafruit MLX90640, Adafruit BusIO
   - ArduinoJson 7.x
   - AsyncTCP + ESPAsyncWebServer (mathieucarbou forks, install from GitHub ZIP;
     your existing ThermaVolt versions will probably work too)

4. Tools menu:
   Board: ESP32S3 Dev Module     PSRAM: OPI PSRAM      Flash size: 8MB (or 16MB to suit)
   Partition scheme: 8M with spiffs (3MB APP)  <- must be an OTA scheme (two app slots); NOT Huge APP     Flash mode: QIO 80MHz
   Core Debug Level: None        (match whatever ThermaVolt uses for USB CDC On Boot)

5. Edit secrets.h for Wi-Fi, or use the AP fallback (ThermalCam-Setup / thermalcam -> http://192.168.4.1/setup).
   The web page is embedded in the firmware (index_html.h) - no filesystem upload needed.

Serial 115200 prints "ui:" progress lines during screen init to help locate any crash.

Web OTA
-------
http://thermalcam.local/update  (or http://<ip>/update, or the link on /setup).
Upload the app binary from Sketch > Export compiled Binary: the file ending .ino.bin
(NOT the .bootloader / .partitions / .merged files). Progress shows on the screen status line.
Set OTA_PASS in config.h to require a login (user = OTA_USER).

Wi-Fi setup on screen
---------------------
If the board can't join Wi-Fi it starts the ThermalCam-Setup AP and an overlay on the
thermal image shows the SSID, password and http://192.168.4.1/setup. It disappears once
the board is on your network.
