#pragma once
#include <Arduino.h>
#include "thermal.h"

void netBegin();                          // Wi-Fi (STA, AP fallback) + web server + WebSocket
void netLoop();
String netIpString();
bool netIsAp();
void webOnFrame(const ThermalFrame &f);   // register with sensorOnFrame(): broadcasts to browsers
int  netOtaPercent();                 // -1 when idle, 0..100 while a web firmware upload is running
