#pragma once
#include "palette.h"

void uiBegin();
void uiLoop();                       // call from loop(): renders newest frame + runs LVGL
void uiSetPalette(PaletteId id);     // thread-safe request (applied in uiLoop)
PaletteId uiGetPalette();
