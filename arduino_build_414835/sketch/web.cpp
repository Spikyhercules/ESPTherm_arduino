#line 1 "C:\\Projects\\esp-thermal-arduino-ide\\esp_thermal\\web.cpp"
#include "web.h"
#include "display_ui.h"
#include "palette.h"
#include "secrets.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include "index_html.h"
#include <Update.h>
#include <Preferences.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");
static Preferences prefs;
static bool s_ap = false;
static uint32_t s_restartAt = 0;
static volatile bool s_otaActive = false;
static volatile bool s_otaOk = false;
static size_t s_otaTotal = 0, s_otaDone = 0;

int netOtaPercent() {
  if (!s_otaActive) return -1;
  if (!s_otaTotal) return 0;
  int p = (int)((uint64_t)s_otaDone * 100 / s_otaTotal);
  return p > 99 ? 99 : p;
}

static bool otaAuth(AsyncWebServerRequest *r) {
  if (!OTA_PASS[0]) return true;
  return r->authenticate(OTA_USER, OTA_PASS);
}

// ---- binary frame layout (little endian) ----
//  0 u8  type (=1)        1 u8 flags (bit0 = sensor ok)   2 u16 seq
//  4 f32 min   8 f32 max  12 f32 avg  16 f32 centre
// 20 u16 hotIdx  22 u16 coldIdx  24 u32 uptime ms
// 28 int16[768] temperatures in 0.01 degC (row-major, 32 x 24)
static constexpr size_t HDR = 28;
static uint8_t s_tx[HDR + TH_N * 2];

String netIpString() {
  if (s_ap) return WiFi.softAPIP().toString();
  if (WiFi.status() == WL_CONNECTED) return WiFi.localIP().toString();
  return "no wifi";
}
bool netIsAp() { return s_ap; }

void webOnFrame(const ThermalFrame &f) {
  if (s_otaActive) return;                  // keep the link clear for the upload
  if (ws.count() == 0 || !ws.availableForWriteAll()) return;
  uint8_t *p = s_tx;
  p[0] = 1; p[1] = f.sensorOk ? 1 : 0;
  uint16_t seq = (uint16_t)f.seq;           memcpy(p + 2, &seq, 2);
  memcpy(p + 4, &f.minT, 4);  memcpy(p + 8, &f.maxT, 4);
  memcpy(p + 12, &f.avgT, 4); memcpy(p + 16, &f.centreT, 4);
  memcpy(p + 20, &f.hotIdx, 2); memcpy(p + 22, &f.coldIdx, 2);
  memcpy(p + 24, &f.ms, 4);
  int16_t *t = (int16_t *)(p + HDR);
  for (int i = 0; i < TH_N; i++) {
    float v = f.temps[i] * 100.0f;
    if (v > 32767) v = 32767; if (v < -32768) v = -32768;
    t[i] = (int16_t)lroundf(v);
  }
  ws.binaryAll(s_tx, sizeof(s_tx));
}

static String helloJson() {
  JsonDocument d;
  d["type"] = "hello"; d["w"] = TH_W; d["h"] = TH_H;
  d["rate"] = sensorGetRate(); d["palette"] = paletteName(uiGetPalette());
  String s; serializeJson(d, s); return s;
}

static void onWsEvent(AsyncWebSocket *, AsyncWebSocketClient *c, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    c->text(helloJson());
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (!(info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)) return;
    JsonDocument d;
    if (deserializeJson(d, data, len)) return;
    const char *cmd = d["cmd"] | "";
    if (!strcmp(cmd, "rate")) sensorSetRate(d["hz"] | DEFAULT_RATE_HZ);
    else if (!strcmp(cmd, "palette")) uiSetPalette(paletteFromName(d["name"] | "", uiGetPalette()));
    ws.textAll(helloJson());
  }
}

static const char SETUP_HTML[] PROGMEM = R"HTML(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Thermal Cam Wi-Fi setup</title>
<style>body{font:16px system-ui;background:#0b0e12;color:#e6edf3;max-width:420px;margin:30px auto;padding:0 16px}
input,button{width:100%;padding:12px;margin:8px 0;border-radius:8px;border:1px solid #2a323d;background:#151a21;color:#e6edf3;font-size:16px}
button{background:#ff8a1f;color:#000;font-weight:600;border:0}a{color:#ff8a1f}</style></head><body>
<h2>Wi-Fi setup</h2><form method="POST" action="/setup">
<input name="ssid" placeholder="Network name (SSID)" required>
<input name="pass" type="password" placeholder="Password">
<button>Save &amp; restart</button></form><p><a href="/">Back to thermal view</a></p></body></html>)HTML";

static const char UPDATE_HTML[] PROGMEM = R"HTML(<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Thermal Cam firmware update</title>
<style>body{font:16px system-ui;background:#0b0e12;color:#e6edf3;max-width:420px;margin:30px auto;padding:0 16px}
input,button{width:100%;padding:12px;margin:8px 0;border-radius:8px;border:1px solid #2a323d;background:#151a21;color:#e6edf3;font-size:16px;box-sizing:border-box}
button{background:#ff8a1f;color:#000;font-weight:600;border:0}a{color:#ff8a1f}
#bar{height:14px;background:#151a21;border-radius:7px;overflow:hidden;margin:12px 0}#fill{height:100%;width:0;background:#ff8a1f}</style></head><body>
<h2>Firmware update</h2>
<p>Choose the app binary (<code>.ino.bin</code> from <i>Sketch &gt; Export compiled Binary</i>, not the bootloader or merged file).</p>
<input type="file" id="f" accept=".bin"><button id="go">Upload &amp; restart</button>
<div id="bar"><div id="fill"></div></div><p id="msg"></p><p><a href="/">Back to thermal view</a></p>
<script>
document.getElementById('go').onclick=function(){
var f=document.getElementById('f').files[0],m=document.getElementById('msg'),b=document.getElementById('fill');
if(!f){m.textContent='Pick a .bin file first';return}
var x=new XMLHttpRequest(),d=new FormData();d.append('firmware',f,f.name);
x.upload.onprogress=function(e){if(e.lengthComputable){b.style.width=(e.loaded*100/e.total)+'%';m.textContent='Uploading '+Math.round(e.loaded*100/e.total)+'%'}};
x.onload=function(){if(x.status==200&&x.responseText=='OK'){b.style.width='100%';m.textContent='Done - restarting. Reloading in 12 s...';setTimeout(function(){location.href='/'},12000)}else m.textContent='Update failed: '+x.responseText};
x.onerror=function(){m.textContent='Connection lost (device may be restarting)'};
x.open('POST','/update');x.send(d);m.textContent='Uploading...'};
</script></body></html>)HTML";

void netBegin() {
  prefs.begin("wifi", true);
  String ssid = prefs.getString("ssid", WIFI_SSID);
  String pass = prefs.getString("pass", WIFI_PASS);
  prefs.end();

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
  WiFi.setSleep(false);
#if USE_STATIC_IP
  WiFi.config(IPAddress(STATIC_IP), IPAddress(STATIC_GW), IPAddress(STATIC_MASK), IPAddress(STATIC_DNS));
#endif
  WiFi.begin(ssid.c_str(), pass.c_str());
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT_MS) {
    uiLoop();             // keep the screen alive while connecting
    delay(100);
  }
  if (WiFi.status() == WL_CONNECTED) {
    MDNS.begin(HOSTNAME);
    MDNS.addService("http", "tcp", 80);
    Serial.printf("WiFi OK  http://%s  (http://%s.local)\n", WiFi.localIP().toString().c_str(), HOSTNAME);
  } else {
    s_ap = true;
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.printf("WiFi failed -> AP '%s'  http://%s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
  }

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *r) {
    r->send_P(200, "text/html", INDEX_HTML);
  });
  server.on("/setup", HTTP_GET, [](AsyncWebServerRequest *r) { r->send(200, "text/html", FPSTR(SETUP_HTML)); });
  server.on("/setup", HTTP_POST, [](AsyncWebServerRequest *r) {
    if (!r->hasParam("ssid", true)) { r->send(400, "text/plain", "missing ssid"); return; }
    String s = r->getParam("ssid", true)->value();
    String p = r->hasParam("pass", true) ? r->getParam("pass", true)->value() : "";
    Preferences pr; pr.begin("wifi", false); pr.putString("ssid", s); pr.putString("pass", p); pr.end();
    r->send(200, "text/html", "<meta name='viewport' content='width=device-width'><body style='font:16px system-ui;background:#0b0e12;color:#e6edf3;padding:30px'>Saved. Restarting - reconnect to your normal Wi-Fi and open http://" HOSTNAME ".local</body>");
    s_restartAt = millis() + 1500;
  });
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *r) {
    if (!otaAuth(r)) { r->requestAuthentication(); return; }
    r->send_P(200, "text/html", UPDATE_HTML);
  });
  server.on("/update", HTTP_POST,
    [](AsyncWebServerRequest *r) {
      if (!otaAuth(r)) { r->requestAuthentication(); return; }
      bool ok = s_otaOk && !Update.hasError();
      s_otaActive = false;
      AsyncWebServerResponse *resp = r->beginResponse(ok ? 200 : 500, "text/plain", ok ? "OK" : "FAIL");
      resp->addHeader("Connection", "close");
      r->send(resp);
      if (ok) s_restartAt = millis() + 1500;
    },
    [](AsyncWebServerRequest *r, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      if (!otaAuth(r)) return;
      if (!index) {
        Serial.printf("OTA start: %s\n", filename.c_str());
        s_otaOk = false; s_otaTotal = r->contentLength(); s_otaDone = 0; s_otaActive = true;
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { Update.printError(Serial); s_otaActive = false; return; }
      }
      if (!s_otaActive || Update.hasError()) return;
      if (Update.write(data, len) != len) { Update.printError(Serial); s_otaActive = false; return; }
      s_otaDone = index + len;
      if (final) {
        if (Update.end(true)) { s_otaOk = true; Serial.println("OTA done"); }
        else { Update.printError(Serial); s_otaActive = false; }
      }
    });
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *r) {
    ThermalFrame *f = new ThermalFrame();
    JsonDocument d;
    if (thermalCopyLatest(*f)) {
      d["sensorOk"] = f->sensorOk; d["fps"] = f->fps; d["min"] = f->minT; d["max"] = f->maxT;
      d["avg"] = f->avgT; d["centre"] = f->centreT;
    }
    delete f;
    d["ip"] = netIpString(); d["ap"] = s_ap; d["rssi"] = WiFi.RSSI();
    d["rate"] = sensorGetRate(); d["palette"] = paletteName(uiGetPalette());
    d["heap"] = ESP.getFreeHeap(); d["psram"] = ESP.getFreePsram(); d["uptime"] = millis() / 1000;
    d["clients"] = ws.count();
    String s; serializeJson(d, s);
    r->send(200, "application/json", s);
  });
  server.onNotFound([](AsyncWebServerRequest *r) { r->send(404, "text/plain", "Not found"); });
  server.begin();
}

void netLoop() {
  static uint32_t lastClean = 0;
  if (millis() - lastClean > 1000) { lastClean = millis(); ws.cleanupClients(); }
  if (s_restartAt && millis() > s_restartAt) ESP.restart();
}
