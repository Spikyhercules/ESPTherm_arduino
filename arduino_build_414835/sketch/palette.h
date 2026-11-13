#line 1 "C:\\Projects\\esp-thermal-arduino-ide\\esp_thermal\\palette.h"
#pragma once
#include <Arduino.h>

enum PaletteId : uint8_t { PAL_IRONBOW = 0, PAL_INFERNO = 1, PAL_COUNT };

void paletteInit();
const uint16_t *paletteRGB565(PaletteId id);   // 256 entries
const char *paletteName(PaletteId id);
PaletteId paletteFromName(const char *name, PaletteId fallback);
