# 1 "C:\\Projects\\esp-thermal-arduino-ide\\esp_thermal\\esp_thermal.ino"
// ESP32-S3 + MLX90640 thermal camera
// One sensor/temperature engine feeds both the LVGL screen and the web page.
# 4 "C:\\Projects\\esp-thermal-arduino-ide\\esp_thermal\\esp_thermal.ino" 2
# 5 "C:\\Projects\\esp-thermal-arduino-ide\\esp_thermal\\esp_thermal.ino" 2
# 6 "C:\\Projects\\esp-thermal-arduino-ide\\esp_thermal\\esp_thermal.ino" 2
# 7 "C:\\Projects\\esp-thermal-arduino-ide\\esp_thermal\\esp_thermal.ino" 2
# 8 "C:\\Projects\\esp-thermal-arduino-ide\\esp_thermal\\esp_thermal.ino" 2

void setup() {
  Serial0.begin(115200);
  delay(300);
  Serial0.println("\nESP32-S3 MLX90640 Thermal Camera");
  Serial0.printf("PSRAM: %u bytes free\n", (unsigned)ESP.getFreePsram());

  thermalInit();
  paletteInit();
  sensorOnFrame(webOnFrame); // web broadcast hook (runs in the sensor task)
  uiBegin();
  sensorBegin();
  netBegin();
}

void loop() {
  uiLoop();
  netLoop();
  delay(5);
}
