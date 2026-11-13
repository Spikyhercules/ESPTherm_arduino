// Local LVGL display (ILI9341 320x240 landscape via TFT_eSPI).
// Layout: interpolated thermal image (224x168, 7x) + colour bar + stats column.
#include "display_ui.h"
#include "thermal.h"
#include "web.h"
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <esp_heap_caps.h>

#define SCR_W 320
#define SCR_H 240
#define IMG_W 224
#define IMG_H 168
#define IMG_X 4
#define IMG_Y 4
#define BAR_Y 174
#define BAR_H 8
#define COL_X 234

static TFT_eSPI tft;
static lv_disp_draw_buf_t s_drawBuf;
static lv_disp_drv_t s_dispDrv;
static lv_color_t *s_buf1, *s_buf2;

static uint16_t *s_img;          // IMG_W x IMG_H RGB565 canvas buffer
static uint16_t *s_bar;          // IMG_W x BAR_H colour bar buffer
static lv_obj_t *s_cvImg, *s_cvBar;
static lv_obj_t *lblMax, *lblMin, *lblAvg, *lblCtr;
static lv_obj_t *lblHot, *lblCold, *lblLo, *lblHi;
static lv_obj_t *lblStatus, *lblPal, *lblFps, *lblIp;
static lv_obj_t *boxAp, *lblAp;          // full-screen Wi-Fi setup page (AP fallback)

static ThermalFrame s_f;                 // UI copy of the newest frame
static float s_disp[TH_N];               // smoothed copy used ONLY for drawing
static bool  s_dispInit = false;
static bool  s_uiOk = true;
static float s_lo = 0, s_hi = 1;
static bool  s_rangeInit = false;

static volatile uint8_t s_palReq = PAL_IRONBOW;
static PaletteId s_pal = PAL_IRONBOW;

// bilinear lookup tables (7x upscale)
static uint8_t s_cx0[IMG_W], s_ry0[IMG_H];
static float   s_cfx[IMG_W], s_rfy[IMG_H];

static void flushCb(lv_disp_drv_t *d, const lv_area_t *a, lv_color_t *c) {
  uint32_t w = a->x2 - a->x1 + 1, h = a->y2 - a->y1 + 1;
  tft.startWrite();
  tft.setAddrWindow(a->x1, a->y1, w, h);
  tft.pushColors((uint16_t *)&c->full, w * h, true);
  tft.endWrite();
  lv_disp_flush_ready(d);
}

static void buildTables() {
  const float sc = (float)IMG_W / TH_W;
  for (int x = 0; x < IMG_W; x++) {
    float s = (x + 0.5f) / sc - 0.5f; if (s < 0) s = 0;
    int x0 = (int)s; float f = s - x0;
    if (x0 >= TH_W - 1) { x0 = TH_W - 1; f = 0; }
    s_cx0[x] = x0; s_cfx[x] = f;
  }
  for (int y = 0; y < IMG_H; y++) {
    float s = (y + 0.5f) / sc - 0.5f; if (s < 0) s = 0;
    int y0 = (int)s; float f = s - y0;
    if (y0 >= TH_H - 1) { y0 = TH_H - 1; f = 0; }
    s_ry0[y] = y0; s_rfy[y] = f;
  }
}

static lv_obj_t *mkLabel(lv_obj_t *p, int x, int y, const lv_font_t *f, uint32_t col, const char *txt) {
  lv_obj_t *l = lv_label_create(p);
  lv_obj_set_pos(l, x, y);
  lv_obj_set_style_text_font(l, f, 0);
  lv_obj_set_style_text_color(l, lv_color_hex(col), 0);
  lv_label_set_text(l, txt);
  return l;
}

static void drawColourBar() {
  const uint16_t *lut = paletteRGB565(s_pal);
  for (int x = 0; x < IMG_W; x++) {
    uint16_t c = lut[x * 255 / (IMG_W - 1)];
    for (int y = 0; y < BAR_H; y++) s_bar[y * IMG_W + x] = c;
  }
  lv_obj_invalidate(s_cvBar);
}

static inline void putPx(int x, int y, uint16_t c) {
  if (x >= 0 && x < IMG_W && y >= 0 && y < IMG_H) s_img[y * IMG_W + x] = c;
}
static void cross(int cx, int cy, int r, uint16_t c) {
  for (int i = -r; i <= r; i++) { putPx(cx + i, cy, c); putPx(cx, cy + i, c); }
}

static void renderImage() {
  const uint16_t *lut = paletteRGB565(s_pal);
  const float inv = 255.0f / (s_hi - s_lo);
  for (int y = 0; y < IMG_H; y++) {
    int y0 = s_ry0[y], y1 = (y0 + 1 < TH_H) ? y0 + 1 : y0;
    float fy = s_rfy[y];
    const float *r0 = &s_disp[y0 * TH_W], *r1 = &s_disp[y1 * TH_W];
    uint16_t *out = &s_img[y * IMG_W];
    for (int x = 0; x < IMG_W; x++) {
      int x0 = s_cx0[x], x1 = (x0 + 1 < TH_W) ? x0 + 1 : x0;
      float fx = s_cfx[x];
      float top = r0[x0] + (r0[x1] - r0[x0]) * fx;
      float bot = r1[x0] + (r1[x1] - r1[x0]) * fx;
      float v = top + (bot - top) * fy;
      int idx = (int)((v - s_lo) * inv);
      if (idx < 0) idx = 0; else if (idx > 255) idx = 255;
      out[x] = lut[idx];
    }
  }
  const float sc = (float)IMG_W / TH_W;
  int hx = (int)(((s_f.hotIdx % TH_W) + 0.5f) * sc), hy = (int)(((s_f.hotIdx / TH_W) + 0.5f) * sc);
  int cx = (int)(((s_f.coldIdx % TH_W) + 0.5f) * sc), cy = (int)(((s_f.coldIdx / TH_W) + 0.5f) * sc);
  cross(IMG_W / 2, IMG_H / 2, 2, 0xFFFF);   // centre reticle
  cross(cx, cy, 4, 0x07FF);                  // coldspot = cyan
  cross(hx, hy, 4, 0xFFFF);                  // hotspot  = white
  lv_obj_invalidate(s_cvImg);
}

static void setStatus(const char *t, uint32_t col) {
  lv_label_set_text(lblStatus, t);
  lv_obj_set_style_text_color(lblStatus, lv_color_hex(col), 0);
}

static void updateFromFrame() {
  if (!s_f.sensorOk) { setStatus("SENSOR FAIL", 0xf85149); return; }
  setStatus("SENSOR OK", 0x3fb950);

  // display-only smoothing: raw temps in s_f are never altered
  if (!s_dispInit) { memcpy(s_disp, s_f.temps, sizeof(s_disp)); s_dispInit = true; }
  else for (int i = 0; i < TH_N; i++) s_disp[i] += UI_SMOOTH * (s_f.temps[i] - s_disp[i]);

  float mn = s_f.minT, mx = s_f.maxT;
  if (mx - mn < UI_MIN_SPAN) { float m = (mx + mn) * 0.5f; mn = m - UI_MIN_SPAN / 2; mx = m + UI_MIN_SPAN / 2; }
  if (!s_rangeInit) { s_lo = mn; s_hi = mx; s_rangeInit = true; }
  else { s_lo += 0.3f * (mn - s_lo); s_hi += 0.3f * (mx - s_hi); }

  renderImage();

  char b[48];
  snprintf(b, sizeof(b), "%.1f", s_f.maxT);    lv_label_set_text(lblMax, b);
  snprintf(b, sizeof(b), "%.1f", s_f.minT);    lv_label_set_text(lblMin, b);
  snprintf(b, sizeof(b), "%.1f", s_f.avgT);    lv_label_set_text(lblAvg, b);
  snprintf(b, sizeof(b), "%.1f", s_f.centreT); lv_label_set_text(lblCtr, b);
  snprintf(b, sizeof(b), "HOT  %.1fC  (%d,%d)", s_f.temps[s_f.hotIdx], s_f.hotIdx % TH_W, s_f.hotIdx / TH_W);
  lv_label_set_text(lblHot, b);
  snprintf(b, sizeof(b), "COLD %.1fC  (%d,%d)", s_f.temps[s_f.coldIdx], s_f.coldIdx % TH_W, s_f.coldIdx / TH_W);
  lv_label_set_text(lblCold, b);
  snprintf(b, sizeof(b), "%.0f", s_lo);        lv_label_set_text(lblLo, b);
  snprintf(b, sizeof(b), "%.0f", s_hi);        lv_label_set_text(lblHi, b);
  snprintf(b, sizeof(b), "%.1f fps", s_f.fps); lv_label_set_text(lblFps, b);
}

void uiSetPalette(PaletteId id) { s_palReq = id; }
PaletteId uiGetPalette() { return (PaletteId)s_palReq; }

void uiBegin() {
  Serial.println("ui: tft.begin");
  buildTables();
  tft.begin();
  Serial.println("ui: tft ok");
  tft.setRotation(UI_ROTATION);
  tft.invertDisplay(UI_INVERT);
  tft.fillScreen(TFT_BLACK);

  lv_init();
  size_t bufPx = SCR_W * 30;
  s_buf1 = (lv_color_t *)heap_caps_malloc(bufPx * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  s_buf2 = (lv_color_t *)heap_caps_malloc(bufPx * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  lv_disp_draw_buf_init(&s_drawBuf, s_buf1, s_buf2, bufPx);
  lv_disp_drv_init(&s_dispDrv);
  s_dispDrv.hor_res = SCR_W;
  s_dispDrv.ver_res = SCR_H;
  s_dispDrv.flush_cb = flushCb;
  s_dispDrv.draw_buf = &s_drawBuf;
  lv_disp_t *disp = lv_disp_drv_register(&s_dispDrv);
  Serial.printf("ui: bufs %p %p disp %p\n", s_buf1, s_buf2, disp);

  s_img = (uint16_t *)heap_caps_malloc(IMG_W * IMG_H * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!s_img) s_img = (uint16_t *)malloc(IMG_W * IMG_H * 2);
  s_bar = (uint16_t *)malloc(IMG_W * BAR_H * 2);
  if (!s_buf1 || !s_buf2 || !disp || !s_img || !s_bar) { Serial.println("ui: init failed"); s_uiOk = false; return; }
  memset(s_img, 0, IMG_W * IMG_H * 2);

  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  s_cvImg = lv_canvas_create(scr);
  lv_canvas_set_buffer(s_cvImg, s_img, IMG_W, IMG_H, LV_IMG_CF_TRUE_COLOR);
  lv_obj_set_pos(s_cvImg, IMG_X, IMG_Y);
  s_cvBar = lv_canvas_create(scr);
  lv_canvas_set_buffer(s_cvBar, s_bar, IMG_W, BAR_H, LV_IMG_CF_TRUE_COLOR);
  lv_obj_set_pos(s_cvBar, IMG_X, BAR_Y);

  const lv_font_t *f12 = &lv_font_montserrat_12, *f14 = &lv_font_montserrat_14, *f20 = &lv_font_montserrat_20;
  const uint32_t GREY = 0x8b98a5, WHITE = 0xFFFFFF;

  // right-hand statistics column
  mkLabel(scr, COL_X, 4,   f12, GREY, "MAX (C)");  lblMax = mkLabel(scr, COL_X, 18,  f20, 0xff8a1f, "--");
  mkLabel(scr, COL_X, 44,  f12, GREY, "MIN (C)");  lblMin = mkLabel(scr, COL_X, 58,  f20, 0x4fc3f7, "--");
  mkLabel(scr, COL_X, 84,  f12, GREY, "AVG (C)");  lblAvg = mkLabel(scr, COL_X, 98,  f20, WHITE, "--");
  mkLabel(scr, COL_X, 124, f12, GREY, "CENTRE (C)"); lblCtr = mkLabel(scr, COL_X, 138, f20, WHITE, "--");

  lblStatus = mkLabel(scr, COL_X, 172, f12, 0xd29922, "NO DATA");
  lblPal    = mkLabel(scr, COL_X, 188, f12, GREY, paletteName(s_pal));
  lblFps    = mkLabel(scr, COL_X, 202, f12, GREY, "-- fps");
  lblIp     = mkLabel(scr, COL_X, 218, f12, GREY, "no wifi");
  lv_label_set_long_mode(lblIp, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_obj_set_width(lblIp, SCR_W - COL_X - 2);

  // colour-scale labels + hot/cold lines (left column)
  lblLo = mkLabel(scr, IMG_X, BAR_Y + BAR_H + 2, f12, GREY, "--");
  lblHi = mkLabel(scr, IMG_X + IMG_W - 60, BAR_Y + BAR_H + 2, f12, GREY, "--");
  lv_obj_set_width(lblHi, 60);
  lv_obj_set_style_text_align(lblHi, LV_TEXT_ALIGN_RIGHT, 0);
  lblHot  = mkLabel(scr, IMG_X, 200, f14, 0xff8a1f, "HOT  --");
  lblCold = mkLabel(scr, IMG_X, 218, f14, 0x4fc3f7, "COLD --");

  // Full-screen Wi-Fi setup page, shown only while in AP fallback mode
  boxAp = lv_obj_create(scr);
  lv_obj_set_size(boxAp, SCR_W, SCR_H);
  lv_obj_set_pos(boxAp, 0, 0);
  lv_obj_clear_flag(boxAp, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(boxAp, lv_color_hex(0x0b0e12), 0);
  lv_obj_set_style_bg_opa(boxAp, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(boxAp, 0, 0);
  lv_obj_set_style_radius(boxAp, 0, 0);
  lv_obj_set_style_pad_all(boxAp, 0, 0);
  auto mid = [&](int y, const lv_font_t *f, uint32_t col, const char *t) {
    lv_obj_t *l = mkLabel(boxAp, 0, y, f, col, t);
    lv_obj_set_width(l, SCR_W);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    return l;
  };
  mid(12,  f20, 0xff8a1f, "Wi-Fi Setup");
  mid(46,  f14, GREY,     "1. Join this Wi-Fi network");
  mid(66,  f20, WHITE,    AP_SSID);
  mid(104, f14, GREY,     "Password");
  mid(124, f20, WHITE,    AP_PASS);
  mid(162, f14, GREY,     "2. Then open in a browser");
  lblAp = mid(182, f20, 0x4fc3f7, "http://192.168.4.1/setup");
  mid(216, f12, GREY,     "This screen closes once Wi-Fi is connected");
  lv_obj_add_flag(boxAp, LV_OBJ_FLAG_HIDDEN);

  drawColourBar();
  Serial.println("ui: ready");
}

void uiLoop() {
  if (!s_uiOk) return;
#if !LV_TICK_CUSTOM
  // lv_conf.h has no tick source, so feed LVGL's clock ourselves
  { static uint32_t lastTick = 0; uint32_t now = millis(); lv_tick_inc(now - lastTick); lastTick = now; }
#endif
  static uint32_t lastSeq = 0, lastSlow = 0;
  bool force = false;

  if ((PaletteId)s_palReq != s_pal) {
    s_pal = (PaletteId)s_palReq;
    lv_label_set_text(lblPal, paletteName(s_pal));
    drawColourBar();
    force = true;
  }
  if (thermalCopyLatest(s_f) && s_f.seq != 0 && (s_f.seq != lastSeq || force)) {
    lastSeq = s_f.seq;
    updateFromFrame();
  }
  if (millis() - lastSlow > 2000) {
    lastSlow = millis();
    String ip = netIpString();
    lv_label_set_text(lblIp, ip.c_str());
    if (netIsAp()) {
      lv_label_set_text_fmt(lblAp, "http://%s/setup", ip.c_str());
      if (lv_obj_has_flag(boxAp, LV_OBJ_FLAG_HIDDEN)) { lv_obj_clear_flag(boxAp, LV_OBJ_FLAG_HIDDEN); lv_obj_move_foreground(boxAp); }
    } else if (!lv_obj_has_flag(boxAp, LV_OBJ_FLAG_HIDDEN)) {
      lv_obj_add_flag(boxAp, LV_OBJ_FLAG_HIDDEN);
    }
  }

  // firmware upload progress shown in the status label
  static int lastOta = -1;
  int ota = netOtaPercent();
  if (ota != lastOta) {
    lastOta = ota;
    if (ota >= 0) { char b[20]; snprintf(b, sizeof(b), "OTA %d%%", ota); setStatus(b, 0xd29922); }
  }
  lv_timer_handler();
}
