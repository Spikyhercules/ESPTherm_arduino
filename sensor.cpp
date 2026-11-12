// MLX90640 driver task + temperature engine.
// Raw readings are cleaned (bad pixels only) and statistics are computed here,
// once, so the LVGL screen and the web page always agree.
#include "thermal.h"
#include <Wire.h>
#include <Adafruit_MLX90640.h>
#include <math.h>

ThermalFrame g_frame;
SemaphoreHandle_t g_frameMutex = nullptr;

static Adafruit_MLX90640 mlx;
static ThermalFrame s_work;
static float s_raw[TH_N];
static volatile uint8_t s_reqRate = DEFAULT_RATE_HZ;
static uint8_t s_appliedRate = 0;
static FrameCb s_cb = nullptr;
static bool s_ready = false;

void thermalInit() {
  if (!g_frameMutex) g_frameMutex = xSemaphoreCreateMutex();
  memset(&g_frame, 0, sizeof(g_frame));
}

bool thermalCopyLatest(ThermalFrame &out) {
  if (!g_frameMutex) return false;
  if (xSemaphoreTake(g_frameMutex, pdMS_TO_TICKS(50)) != pdTRUE) return false;
  out = g_frame;
  xSemaphoreGive(g_frameMutex);
  return true;
}

void sensorOnFrame(FrameCb cb) { s_cb = cb; }

void sensorSetRate(uint8_t hz) {
  if (hz != 1 && hz != 2 && hz != 4 && hz != 8) hz = DEFAULT_RATE_HZ;
  s_reqRate = hz;
}
uint8_t sensorGetRate() { return s_reqRate; }

static mlx90640_refreshrate_t rateEnum(uint8_t hz) {
  switch (hz) {
    case 1: return MLX90640_1_HZ;
    case 2: return MLX90640_2_HZ;
    case 8: return MLX90640_8_HZ;
    default: return MLX90640_4_HZ;
  }
}

static inline bool validT(float t) { return !isnan(t) && t > -40.0f && t < 400.0f; }

// Replace invalid pixels with the mean of valid neighbours. Returns false if the frame is unusable.
static bool cleanFrame(float *t) {
  static bool bad[TH_N];
  static float tmp[TH_N];
  float sum = 0; int cnt = 0;
  for (int i = 0; i < TH_N; i++) {
    bad[i] = !validT(t[i]);
    if (!bad[i]) { sum += t[i]; cnt++; }
  }
  if (cnt < TH_N / 2) return false;
  if (cnt == TH_N) return true;
  float mean = sum / cnt;
  memcpy(tmp, t, sizeof(tmp));
  for (int i = 0; i < TH_N; i++) {
    if (!bad[i]) continue;
    int y = i / TH_W, x = i % TH_W; float s = 0; int n = 0;
    for (int dy = -1; dy <= 1; dy++)
      for (int dx = -1; dx <= 1; dx++) {
        int nx = x + dx, ny = y + dy;
        if (nx < 0 || ny < 0 || nx >= TH_W || ny >= TH_H) continue;
        int j = ny * TH_W + nx;
        if (!bad[j]) { s += t[j]; n++; }
      }
    tmp[i] = n ? s / n : mean;
  }
  memcpy(t, tmp, sizeof(tmp));
  return true;
}

static void computeStats(ThermalFrame &f) {
  float mn = f.temps[0], mx = mn, sum = 0;
  uint16_t hi = 0, ci = 0;
  for (int i = 0; i < TH_N; i++) {
    float t = f.temps[i];
    sum += t;
    if (t > mx) { mx = t; hi = i; }
    if (t < mn) { mn = t; ci = i; }
  }
  f.minT = mn; f.maxT = mx; f.avgT = sum / TH_N;
  f.centreT = f.temps[(TH_H / 2) * TH_W + TH_W / 2];   // pixel (16,12)
  f.hotIdx = hi; f.coldIdx = ci;
}

static void i2cScan() {
  int found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) { Serial.printf("  I2C device at 0x%02X\n", a); found++; }
  }
  if (!found) Serial.println("  I2C scan: nothing found");
}

static bool initSensor() {
  static uint32_t lastDiag = 0;
  Wire.beginTransmission(MLX90640_I2CADDR_DEFAULT);
  uint8_t e = Wire.endTransmission();
  if (e != 0) {
    if (millis() - lastDiag > 5000) {
      lastDiag = millis();
      Serial.printf("MLX90640: no ACK at 0x33 (err %u). SDA=GPIO%d SCL=GPIO%d - check wiring, 3V3, pull-ups\n",
                    e, PIN_I2C_SDA, PIN_I2C_SCL);
      i2cScan();
    }
    return false;
  }
  if (!mlx.begin(MLX90640_I2CADDR_DEFAULT, &Wire)) {
    if (millis() - lastDiag > 5000) { lastDiag = millis(); Serial.println("MLX90640: ACKs but begin() failed (EEPROM/param read) - try lower I2C clock"); }
    return false;
  }
  Serial.println("MLX90640: init OK");
  mlx.setMode(MLX90640_CHESS);
  mlx.setResolution(MLX90640_ADC_18BIT);
  mlx.setRefreshRate(rateEnum(s_reqRate));
  s_appliedRate = s_reqRate;
  return true;
}

static void publishFail() {
  xSemaphoreTake(g_frameMutex, portMAX_DELAY);
  g_frame.sensorOk = false;
  g_frame.seq++;
  s_work = g_frame;
  xSemaphoreGive(g_frameMutex);
  if (s_cb) s_cb(s_work);
}

static void sensorTask(void *) {
  uint32_t lastPub = 0; float fps = 0; int fails = 0;
  for (;;) {
    if (!s_ready) {
      if (initSensor()) { s_ready = true; fails = 0; }
      else { publishFail(); vTaskDelay(pdMS_TO_TICKS(1000)); continue; }
    }
    uint8_t want = s_reqRate;
    if (want != s_appliedRate) { mlx.setRefreshRate(rateEnum(want)); s_appliedRate = want; }

    // MLX90640 chess mode: each getFrame() delivers one subpage; two calls = one full frame.
    bool ok = true;
    for (int sp = 0; sp < 2 && ok; sp++) ok = (mlx.getFrame(s_raw) == 0);
    if (ok) {
      memcpy(s_work.temps, s_raw, sizeof(s_raw));
      ok = cleanFrame(s_work.temps);
    }
    if (!ok) {
      if (++fails >= 3) { s_ready = false; publishFail(); }
      vTaskDelay(pdMS_TO_TICKS(50));
      continue;
    }
    fails = 0;
    computeStats(s_work);

    uint32_t now = millis();
    if (lastPub) {
      float inst = 1000.0f / (float)(now - lastPub);
      fps = (fps == 0) ? inst : fps * 0.8f + inst * 0.2f;
    }
    lastPub = now;
    s_work.fps = fps; s_work.ms = now; s_work.sensorOk = true;

    xSemaphoreTake(g_frameMutex, portMAX_DELAY);
    s_work.seq = g_frame.seq + 1;
    g_frame = s_work;
    xSemaphoreGive(g_frameMutex);

    if (s_cb) s_cb(s_work);
  }
}

void sensorBegin() {
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(I2C_CLOCK_HZ);
  xTaskCreatePinnedToCore(sensorTask, "sensor", 16384, nullptr, 2, nullptr, 0);
}
