#line 1 "C:\\Projects\\esp-thermal-arduino-ide\\esp_thermal\\palette.cpp"
#include "palette.h"

// Key colours (position 0..1, R, G, B). Same keys are used by the web page.
struct Key { float p; uint8_t r, g, b; };

static const Key IRONBOW[] = {
  {0.00f,   0,   0,   0}, {0.20f,  30,   0, 110}, {0.40f, 150,   0, 130},
  {0.55f, 210,  30,  60}, {0.70f, 245, 110,   0}, {0.85f, 255, 210,  40},
  {1.00f, 255, 255, 255}};

static const Key INFERNO[] = {
  {0.0f,   0,   0,   4}, {0.1f,  22,  11,  57}, {0.2f,  66,  10, 104},
  {0.3f, 106,  23, 110}, {0.4f, 147,  38, 103}, {0.5f, 188,  55,  84},
  {0.6f, 221,  81,  58}, {0.7f, 243, 120,  25}, {0.8f, 252, 165,  10},
  {0.9f, 246, 215,  70}, {1.0f, 252, 255, 164}};

static uint16_t s_lut[PAL_COUNT][256];

static void build(uint16_t *lut, const Key *k, int n) {
  for (int i = 0; i < 256; i++) {
    float t = i / 255.0f;
    int s = 0;
    while (s < n - 2 && t > k[s + 1].p) s++;
    float f = (t - k[s].p) / (k[s + 1].p - k[s].p);
    auto mix = [&](uint8_t a, uint8_t b) { return (uint8_t)(a + (b - a) * f + 0.5f); };
    uint8_t r = mix(k[s].r, k[s + 1].r), g = mix(k[s].g, k[s + 1].g), b = mix(k[s].b, k[s + 1].b);
    lut[i] = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  }
}

void paletteInit() {
  build(s_lut[PAL_IRONBOW], IRONBOW, sizeof(IRONBOW) / sizeof(Key));
  build(s_lut[PAL_INFERNO], INFERNO, sizeof(INFERNO) / sizeof(Key));
}

const uint16_t *paletteRGB565(PaletteId id) { return s_lut[id < PAL_COUNT ? id : 0]; }
const char *paletteName(PaletteId id) { return id == PAL_INFERNO ? "inferno" : "ironbow"; }
PaletteId paletteFromName(const char *n, PaletteId fb) {
  if (!n) return fb;
  if (!strcmp(n, "inferno")) return PAL_INFERNO;
  if (!strcmp(n, "ironbow")) return PAL_IRONBOW;
  return fb;
}
