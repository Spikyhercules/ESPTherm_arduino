#line 1 "C:\\Projects\\esp-thermal-arduino-ide\\esp_thermal\\thermal.h"
#pragma once
#include <Arduino.h>
#include "config.h"

constexpr int TH_W = 32;
constexpr int TH_H = 24;
constexpr int TH_N = TH_W * TH_H;   // 768

// One processed sensor frame. temps[] are the real sensor-derived values;
// nothing downstream (interpolation, smoothing, palettes) ever modifies them.
struct ThermalFrame {
  float    temps[TH_N];
  float    minT, maxT, avgT, centreT;
  uint16_t hotIdx, coldIdx;
  uint32_t seq;
  uint32_t ms;
  float    fps;
  bool     sensorOk;
};

typedef void (*FrameCb)(const ThermalFrame &);

void thermalInit();                          // create shared state (call first)
void sensorOnFrame(FrameCb cb);              // called from the sensor task for each new frame
void sensorBegin();                          // start the sensor task
void sensorSetRate(uint8_t hz);              // 1,2,4,8
uint8_t sensorGetRate();
bool thermalCopyLatest(ThermalFrame &out);   // thread-safe snapshot of latest frame
