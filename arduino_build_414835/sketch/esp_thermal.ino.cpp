#line 1 "C:\\Projects\\esp-thermal-arduino-ide\\esp_thermal\\esp_thermal.ino"
// ESP32-S3 + MLX90640 thermal camera
// One sensor/temperature engine feeds both the LVGL screen and the web page.
#include <Arduino.h>
#include "thermal.h"
#include "palette.h"
#include "display_ui.h"
#include "web.h"

#line 9 "C:\\Projects\\esp-thermal-arduino-ide\\esp_thermal\\esp_thermal.ino"
void setup();
#line 23 "C:\\Projects\\esp-thermal-arduino-ide\\esp_thermal\\esp_thermal.ino"
void loop();
#line 9 "C:\\Projects\\esp-thermal-arduino-ide\\esp_thermal\\esp_thermal.ino"
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\nESP32-S3 MLX90640 Thermal Camera");
  Serial.printf("PSRAM: %u bytes free\n", (unsigned)ESP.getFreePsram());

  thermalInit();
  paletteInit();
  sensorOnFrame(webOnFrame);   // web broadcast hook (runs in the sensor task)
  uiBegin();
  sensorBegin();
  netBegin();
}

void loop() {
  uiLoop();
  netLoop();
  delay(5);
}

