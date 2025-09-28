#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <SD_MMC.h>
#include <ESPmDNS.h>
#include <ElegantOTA.h>
#include <time.h>
#include <vector>

// --- Camera model AI‑Thinker ESP32‑CAM pin definitions ---
#define CAMERA_MODEL_AI_THINKER
#if defined(CAMERA_MODEL_AI_THINKER)
  #define PWDN_GPIO_NUM     32
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22
#else
  #error "Camera model not defined"
#endif

#define LED_PIN 4  // on‑board flash LED
const char* MDNS_HOSTNAME = "esp32cam2"; // mDNS hostname (use lowercase)----------------------------------Change Per Device 
const char* OTA_USER = "esp32cam2"; // OTA basic auth username----------------------------------Change Per Device 
const char* OTA_PASS = "1234578"; // OTA basic auth password----------------------------------Change Per Device 

WebServer server(80);
Preferences preferences;

struct WiFiCredential {
  String ssid, password, localIP;
};
std::vector<WiFiCredential> savedNetworks;
bool isAP = false;
bool cameraReady = false;
bool sdMounted = false;
bool restartPending = false;
unsigned long restartAtMs = 0;
bool otaInProgress = false;

void initCamera() {
  camera_config_t cfg = {};
  cfg.ledc_channel = LEDC_CHANNEL_0; cfg.ledc_timer = LEDC_TIMER_0;
  cfg.pin_d0 = Y2_GPIO_NUM;  cfg.pin_d1 = Y3_GPIO_NUM;
  cfg.pin_d2 = Y4_GPIO_NUM;  cfg.pin_d3 = Y5_GPIO_NUM;
  cfg.pin_d4 = Y6_GPIO_NUM;  cfg.pin_d5 = Y7_GPIO_NUM;
  cfg.pin_d6 = Y8_GPIO_NUM;  cfg.pin_d7 = Y9_GPIO_NUM;
  cfg.pin_xclk = XCLK_GPIO_NUM; cfg.pin_pclk = PCLK_GPIO_NUM;
  cfg.pin_vsync = VSYNC_GPIO_NUM; cfg.pin_href = HREF_GPIO_NUM;
  cfg.pin_sscb_sda = SIOD_GPIO_NUM; cfg.pin_sscb_scl = SIOC_GPIO_NUM;
  cfg.pin_pwdn = PWDN_GPIO_NUM;     cfg.pin_reset = RESET_GPIO_NUM;
  cfg.xclk_freq_hz = 20000000;      cfg.pixel_format = PIXFORMAT_JPEG;
  if(psramFound()){
    cfg.frame_size   = FRAMESIZE_VGA;
    cfg.jpeg_quality = 10;
    cfg.fb_count     = 2;
  } else {
    cfg.frame_size   = FRAMESIZE_VGA;
    cfg.jpeg_quality = 12;
    cfg.fb_count     = 1;
  }
  esp_err_t err = esp_camera_init(&cfg);
  if (err != ESP_OK) {
    Serial.printf("❌ Camera init failed: 0x%x\n", err);
    cameraReady = false;
    return;
  }
  cameraReady = true;
}

void loadNetworks() {
  String data = preferences.getString("networks", "");
  savedNetworks.clear();
  int idx = 0;
  while (idx < data.length()) {
    int end = data.indexOf(';', idx);
    if (end < 0) break;
    String e = data.substring(idx, end);
    int p1 = e.indexOf(':'), p2 = e.indexOf(':', p1 + 1);
    WiFiCredential c;
    c.ssid     = e.substring(0, p1);
    c.password = (p1 < 0 || p2 < 0) ? "" : e.substring(p1 + 1, p2);
    c.localIP  = (p2 < 0) ? "" : e.substring(p2 + 1);
    savedNetworks.push_back(c);
    idx = end + 1;
  }
}

void saveNetworks() {
  String out;
  for (auto &n : savedNetworks) {
    out += n.ssid + ":" + n.password + ":" + n.localIP + ";";
  }
  preferences.putString("networks", out);
}

void loadCameraSettings() {
  String s = preferences.getString("camSettings", "");
  sensor_t *sensor = esp_camera_sensor_get();
  if (!sensor || s.length() == 0) return;
  int vals[16]; int n = 0;
  int start = 0;
  for (int i = 0; i <= s.length() && n < 16; i++) {
    if (i == s.length() || s.charAt(i) == ',') {
      vals[n++] = s.substring(start, i).toInt();
      start = i + 1;
    }
  }
  if (n >= 1) sensor->set_framesize(sensor, (framesize_t)vals[0]);
  if (n >= 2) sensor->set_brightness(sensor, vals[1]);
  if (n >= 3) sensor->set_contrast(sensor, vals[2]);
  if (n >= 4) sensor->set_saturation(sensor, vals[3]);
  if (n >= 5) sensor->set_whitebal(sensor, vals[4]);
  if (n >= 6) sensor->set_quality(sensor, vals[5]);
  if (n >= 7) sensor->set_hmirror(sensor, vals[6]);
  if (n >= 8) sensor->set_vflip(sensor, vals[7]);
  if (n >= 9) sensor->set_gain_ctrl(sensor, vals[8]);
  if (n >= 10) sensor->set_gainceiling(sensor, (gainceiling_t)vals[9]);
  if (n >= 11) sensor->set_aec2(sensor, vals[10]);
  if (n >= 12) sensor->set_ae_level(sensor, vals[11]);
  if (n >= 13) sensor->set_awb_gain(sensor, vals[12]);
  if (n >= 14) sensor->set_special_effect(sensor, vals[13]);
  if (n >= 15) sensor->set_sharpness(sensor, vals[14]);
}

String renderMainPage() {
  String bars = "";
  if (!isAP && WiFi.status() == WL_CONNECTED) {
    int r = WiFi.RSSI();
    bars = (r < -80) ? "▂" : (r < -70) ? "▂▄" : (r < -60) ? "▂▄▅" : (r < -50) ? "▂▄▅▆" : "▂▄▅▆▇";
  }
  String html = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<style>
  body{margin:0;font-family:sans-serif;background:#f5f6fa;color:#333;}
  header{background:#333;color:#fff;padding:1rem;text-align:center;}
  .container{padding:1rem;}
  .section{background:#fff;border-radius:8px;padding:1rem;margin-bottom:1rem;box-shadow:0 1px 4px rgba(0,0,0,0.1);}
  h2{margin-top:0;border-bottom:2px solid #0066cc;padding-bottom:0.5rem;}
  img{width:100%;border-radius:8px;}
  .buttons{margin-top:0.5rem;}
  .btn{background:#0066cc;color:#fff;border:none;padding:0.5rem 1rem;margin-right:0.5rem;cursor:pointer;border-radius:4px;}
  .btn:hover{background:#004999;}
  input,select{width:100%;padding:0.5rem;margin:0.5rem 0;border:1px solid #ccc;border-radius:4px;}
  .card{display:flex;justify-content:space-between;padding:0.5rem;align-items:center;border-bottom:1px solid #eee;}
  .card button{background:#cc0000;border:none;color:#fff;padding:0.3rem 0.6rem;border-radius:4px;cursor:pointer;}
  .card button:hover{background:#990000;}
</style>
<script>
  function toggleFS(){
    let e=document.getElementById('stream');
    if(!document.fullscreenElement) e.requestFullscreen();
    else document.exitFullscreen();
  }
  async function captureAndRefresh(){
    await fetch('/capture');
    location.reload();
  }
</script>
<title>ESP32-CAM</title></head><body>
<header><h1>ESP32-CAM Server</h1>)rawliteral"
    + (isAP
       ? "<p>⚠️ Fallback AP Mode Active</p>"
       : "<p>📶 Station: " + WiFi.SSID() + " " + bars + "</p>")
    + R"rawliteral(
</header><div class="container">
  <div class="section">
    <h2>Live Stream</h2>
    <img id="stream" src="/stream" alt="camera stream"/>
    <div class="buttons">
      <button class="btn" onclick="toggleFS()">Full Screen</button>
      <button class="btn" onclick="captureAndRefresh()">Capture & Save</button>
    </div>
  </div>
  <div class="section">
    <h2>Camera Settings</h2>
)rawliteral";

  // Determine current values from saved preferences
  int curFs = 3, curBr = 0, curCt = 0, curSat = 0, curWb = 1;
  int curQ = 10, curHM = 0, curVF = 0, curAGC = 1, curGC = 0, curAEC2 = 0, curAELevel = 0, curAWBGain = 1, curEffect = 0, curSharp = 0;
  String cs = preferences.getString("camSettings", "");
  if (cs.length()) {
    int vals[16]; int n = 0; int start = 0;
    for (int i = 0; i <= cs.length() && n < 16; i++) {
      if (i == cs.length() || cs.charAt(i) == ',') { vals[n++] = cs.substring(start, i).toInt(); start = i + 1; }
    }
    if (n >= 1) curFs = vals[0];
    if (n >= 2) curBr = vals[1];
    if (n >= 3) curCt = vals[2];
    if (n >= 4) curSat = vals[3];
    if (n >= 5) curWb = vals[4];
    if (n >= 6) curQ = vals[5];
    if (n >= 7) curHM = vals[6];
    if (n >= 8) curVF = vals[7];
    if (n >= 9) curAGC = vals[8];
    if (n >= 10) curGC = vals[9];
    if (n >= 11) curAEC2 = vals[10];
    if (n >= 12) curAELevel = vals[11];
    if (n >= 13) curAWBGain = vals[12];
    if (n >= 14) curEffect = vals[13];
    if (n >= 15) curSharp = vals[14];
  }

  html += String(
    "<form action=\"/setCamera\" method=\"GET\">"
    "Framesize<select name=\"framesize\">"
  );
  html += String("<option value=\"0\"") + (curFs==0?" selected":"") + ">QQVGA</option>";
  html += String("<option value=\"1\"") + (curFs==1?" selected":"") + ">QVGA</option>";
  html += String("<option value=\"2\"") + (curFs==2?" selected":"") + ">CIF</option>";
  html += String("<option value=\"3\"") + (curFs==3?" selected":"") + ">VGA</option>";
  html += String("<option value=\"4\"") + (curFs==4?" selected":"") + ">SVGA</option>";
  html += String("<option value=\"5\"") + (curFs==5?" selected":"") + ">XGA</option>";
  html += String("<option value=\"6\"") + (curFs==6?" selected":"") + ">HD</option>";
  html += String("<option value=\"7\"") + (curFs==7?" selected":"") + ">SXGA</option>";
  html += String("<option value=\"8\"") + (curFs==8?" selected":"") + ">UXGA</option>";
  html += "</select>";

  html += "<label>Brightness: <output id=\"brOut\">" + String(curBr) + "</output></label>";
  html += "<input type=\"range\" name=\"brightness\" min=\"-2\" max=\"2\" step=\"1\" value=\"" + String(curBr) + "\" oninput=\"document.getElementById('brOut').textContent=this.value\"/>";

  html += "<label>Contrast: <output id=\"ctOut\">" + String(curCt) + "</output></label>";
  html += "<input type=\"range\" name=\"contrast\" min=\"-2\" max=\"2\" step=\"1\" value=\"" + String(curCt) + "\" oninput=\"document.getElementById('ctOut').textContent=this.value\"/>";

  html += "<label>Saturation: <output id=\"satOut\">" + String(curSat) + "</output></label>";
  html += "<input type=\"range\" name=\"saturation\" min=\"-2\" max=\"2\" step=\"1\" value=\"" + String(curSat) + "\" oninput=\"document.getElementById('satOut').textContent=this.value\"/>";

  html += "<label><input type=\"checkbox\" name=\"whitebal\" value=\"1\" " + String(curWb?"checked":"") + "> Auto White Balance</label>";

  // Additional controls
  html += "<label>JPEG Quality (lower is better): <output id=\"qOut\">" + String(curQ) + "</output></label>";
  html += "<input type=\"range\" name=\"quality\" min=\"4\" max=\"63\" step=\"1\" value=\"" + String(curQ) + "\" oninput=\"document.getElementById('qOut').textContent=this.value\"/>";

  html += "<label><input type=\"checkbox\" name=\"hmirror\" value=\"1\" " + String(curHM?"checked":"") + "> Mirror (Horizontal)</label>";
  html += "<label><input type=\"checkbox\" name=\"vflip\" value=\"1\" " + String(curVF?"checked":"") + "> Flip (Vertical)</label>";

  html += "<label><input type=\"checkbox\" name=\"agc\" value=\"1\" " + String(curAGC?"checked":"") + "> Auto Gain Control</label>";
  html += "<label>Gain Ceiling<select name=\"gainceiling\">";
  for (int gc = 0; gc <= 6; gc++) {
    html += String("<option value=\"") + String(gc) + "\"" + (curGC==gc?" selected":"") + ">" + String(gc) + "</option>";
  }
  html += "</select></label>";

  html += "<label><input type=\"checkbox\" name=\"aec2\" value=\"1\" " + String(curAEC2?"checked":"") + "> AEC2</label>";
  html += "<label>AE Level: <output id=\"aeOut\">" + String(curAELevel) + "</output></label>";
  html += "<input type=\"range\" name=\"ae_level\" min=\"-2\" max=\"2\" step=\"1\" value=\"" + String(curAELevel) + "\" oninput=\"document.getElementById('aeOut').textContent=this.value\"/>";

  html += "<label><input type=\"checkbox\" name=\"awb_gain\" value=\"1\" " + String(curAWBGain?"checked":"") + "> AWB Gain</label>";

  html += "<label>Special Effect<select name=\"effect\">";
  const char* effectNames[] = {"None","Negative","Grayscale","Red Tint","Green Tint","Blue Tint","Sepia","Film","Warm","Cool"};
  for (int e = 0; e <= 9; e++) {
    html += String("<option value=\"") + String(e) + "\"" + (curEffect==e?" selected":"") + ">" + String(effectNames[e]) + "</option>";
  }
  html += "</select></label>";

  html += "<label>Sharpness: <output id=\"shOut\">" + String(curSharp) + "</output></label>";
  html += "<input type=\"range\" name=\"sharpness\" min=\"-3\" max=\"3\" step=\"1\" value=\"" + String(curSharp) + "\" oninput=\"document.getElementById('shOut').textContent=this.value\"/>";

  html += "<div class=\"buttons\"><button type=\"submit\" class=\"btn\">Apply & Save</button></div>";
  html += "</form>";

  html += R"rawliteral(
  </div>
  <div class="section">
    <h2>Saved Networks</h2>)rawliteral";
  if (savedNetworks.empty()) {
    html += "<p>No saved networks.</p>";
  } else {
    for (auto &n : savedNetworks) {
      html += "<div class='card'><span>" + n.ssid + " - " + (n.localIP.length() ? n.localIP : "N/A") + "</span>"
           + "<form action='/deleteNetwork' method='POST' style='margin:0;'>"
           + "<input type='hidden' name='ssid' value='" + n.ssid + "'>"
           + "<button>Delete</button></form></div>";
    }
  }
  html += R"rawliteral(
  </div>
  <div class="section">
    <h2>Add Network</h2>
    <form action="/addNetwork" method="POST">
      <input type="text" name="ssid" placeholder="SSID" required/>
      <input type="text" name="password" placeholder="Password" required/>
      <div class="buttons"><button class="btn" type="submit">Add Network</button></div>
    </form>
  </div>
  <div class="section">
    <h2>System</h2>
    <form action="/restart" method="POST" onsubmit="return confirm('Restart device now?');">
      <div class="buttons"><button class="btn" type="submit" style="background:#cc6600">Restart ESP32</button></div>
    </form>
    <div class="buttons" style="margin-top:0.5rem">
      <a class="btn" href="/update" style="display:inline-block;text-decoration:none">OTA Update</a>
    </div>
  </div>
</div></body></html>)rawliteral";
  return html;
}

void handleRoot()       { server.send(200, "text/html", renderMainPage()); }
void handleJPGStream() {
  if (!cameraReady) { server.send(503, "text/plain", "Camera not initialized"); return; }
  WiFiClient client = server.client();
  server.sendContent(
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n"
  );
  while (client.connected() && !otaInProgress) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) break;
    server.sendContent("--frame\r\nContent-Type: image/jpeg\r\n\r\n");
    client.write(fb->buf, fb->len);
    server.sendContent("\r\n");
    esp_camera_fb_return(fb);
    delay(1); // yield to keep WDT happy during continuous stream
  }
}
void handleCapture() {
  if (!cameraReady) { server.send(503, "text/plain", "Camera not initialized"); return; }
  if (!sdMounted)  { server.send(503, "text/plain", "SD not mounted"); return; }
  camera_fb_t *fb = esp_camera_fb_get();
  if (fb) {
    time_t now = time(nullptr);
    struct tm tm; localtime_r(&now, &tm);
    char fn[32]; strftime(fn, sizeof(fn), "/%m-%d-%Y_%H-%M-%S.jpg", &tm);
    File f = SD_MMC.open(fn, FILE_WRITE);
    if (f) {
      f.write(fb->buf, fb->len);
      f.close();
      Serial.printf("📸 Picture taken and saved: %s\n", fn);
    } else {
      Serial.printf("❌ Failed to save picture %s\n", fn);
    }
    esp_camera_fb_return(fb);
  }
  server.send(204, "text/plain", "");
}
void handleSetCamera() {
  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    int fs  = server.arg("framesize").toInt();
    int br  = server.arg("brightness").toInt();
    int ct  = server.arg("contrast").toInt();
    int sat = server.arg("saturation").toInt();
    int wb  = server.hasArg("whitebal") ? 1 : 0; // checkbox handling
    int q   = server.hasArg("quality") ? server.arg("quality").toInt() : 10;
    int hm  = server.hasArg("hmirror") ? 1 : 0;
    int vf  = server.hasArg("vflip") ? 1 : 0;
    int agc = server.hasArg("agc") ? 1 : 0;
    int gc  = server.hasArg("gainceiling") ? server.arg("gainceiling").toInt() : 0;
    int aec2= server.hasArg("aec2") ? 1 : 0;
    int aeL = server.hasArg("ae_level") ? server.arg("ae_level").toInt() : 0;
    int awbg= server.hasArg("awb_gain") ? 1 : 0;
    int eff = server.hasArg("effect") ? server.arg("effect").toInt() : 0;
    int shp = server.hasArg("sharpness") ? server.arg("sharpness").toInt() : 0;
    s->set_framesize(s, (framesize_t)fs);
    s->set_brightness(s, br);
    s->set_contrast(s, ct);
    s->set_saturation(s, sat);
    s->set_whitebal(s, wb);
    s->set_quality(s, q);
    s->set_hmirror(s, hm);
    s->set_vflip(s, vf);
    s->set_gain_ctrl(s, agc);
    s->set_gainceiling(s, (gainceiling_t)gc);
    s->set_aec2(s, aec2);
    s->set_ae_level(s, aeL);
    s->set_awb_gain(s, awbg);
    s->set_special_effect(s, eff);
    s->set_sharpness(s, shp);
    preferences.putString("camSettings",
      String(fs)+","+String(br)+","+String(ct)+","+String(sat)+","+String(wb)+","+
      String(q)+","+String(hm)+","+String(vf)+","+String(agc)+","+String(gc)+","+
      String(aec2)+","+String(aeL)+","+String(awbg)+","+String(eff)+","+String(shp)
    );
  }
  server.sendHeader("Location","/"); server.send(302, "text/plain", "");
}
void handleRestart() {
  Serial.println("🔄 Restart requested via web UI");
  server.sendHeader("Connection", "close");
  server.send(200, "text/plain", "Restarting...");
  restartAtMs = millis() + 200;
  restartPending = true;
}
void handleAddNetwork() {
  String ss = server.arg("ssid"), pw = server.arg("password");
  Serial.printf("🔄 Trying to add network: %s\n", ss.c_str());
  for (auto &n : savedNetworks) if (n.ssid == ss) {
      server.sendHeader("Location","/"); server.send(302,"",""); 
      return;
  }
  savedNetworks.push_back({ss, pw, ""});
  saveNetworks();
  server.sendHeader("Location","/"); server.send(302, "text/plain", "");
}
void handleDeleteNetwork() {
  String ss = server.arg("ssid");
  Serial.printf("🗑️ Deleting network: %s\n", ss.c_str());
  for (size_t i = 0; i < savedNetworks.size(); i++) {
    if (savedNetworks[i].ssid == ss) {
      savedNetworks.erase(savedNetworks.begin() + i);
      break;
    }
  }
  saveNetworks();
  server.sendHeader("Location","/"); server.send(302, "text/plain", "");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  sdMounted = SD_MMC.begin();
  if (!sdMounted) Serial.println("⚠️ SD_MMC Mount Failed");

  configTzTime("EST5EDT,M3.2.0/2,M11.1.0/2","pool.ntp.org");
  preferences.begin("my-app", false);

  WiFi.mode(WIFI_STA);
  initCamera();
  loadCameraSettings();
  loadNetworks();

  bool connected = false;
  for (auto &n : savedNetworks) {
    Serial.printf("🔄 Connecting to: %s\n", n.ssid.c_str());
    WiFi.disconnect(true); delay(100);
    WiFi.begin(n.ssid.c_str(), n.password.c_str());
    unsigned long start = millis();
    while (millis() - start < 10000 && WiFi.status() != WL_CONNECTED) {
      delay(500);
    }
    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      Serial.printf("✅ Connected to Wi‑Fi: %s\n", n.ssid.c_str());
      Serial.printf("🌐 Local IP: %s\n", WiFi.localIP().toString().c_str());
      n.localIP = WiFi.localIP().toString();
      saveNetworks();

      // 🌟 Blink LED ON/OFF 5 times (500ms each)
      for (int i = 0; i < 5; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(500);
        digitalWrite(LED_PIN, LOW);
        delay(500);
      }

      break;
    }
  }

  if (!connected) {
    Serial.println("⚠️ No Wi‑Fi: Starting fallback AP mode");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32-CAM-Fallback", "esp32cam");
    isAP = true;
    digitalWrite(LED_PIN, HIGH);  // solid ON in AP mode
    Serial.printf("🌐 AP IP: %s\n", WiFi.softAPIP().toString().c_str());
  }

  server.on("/",              HTTP_GET,  handleRoot);
  server.on("/stream",        HTTP_GET,  handleJPGStream);
  server.on("/capture",       HTTP_GET,  handleCapture);
  server.on("/setCamera",     HTTP_GET,  handleSetCamera);
  server.on("/addNetwork",    HTTP_POST, handleAddNetwork);
  server.on("/deleteNetwork", HTTP_POST, handleDeleteNetwork);
  server.on("/restart",       HTTP_POST, handleRestart);

  ElegantOTA.begin(&server);
  ElegantOTA.setAuth(OTA_USER, OTA_PASS);
  ElegantOTA.onStart([]() {
    otaInProgress = true;
    Serial.println("🔄 OTA Start");
  });
  ElegantOTA.onProgress([](size_t current, size_t final) {
    static unsigned long lastLog = 0;
    if (millis() - lastLog > 500) {
      lastLog = millis();
      Serial.printf("🔄 OTA %u/%u (%.0f%%)\n", (unsigned)current, (unsigned)final, final ? (100.0 * current / final) : 0.0);
    }
  });
  ElegantOTA.onEnd([](bool success) {
    Serial.println(success ? "✅ OTA Success" : "❌ OTA Failed");
    otaInProgress = false;
    if (success) {
      restartAtMs = millis() + 300;
      restartPending = true;
    }
  });

  server.begin();
  Serial.println("🚀 Web server started");

  if (!MDNS.begin(MDNS_HOSTNAME)) {
    Serial.println("⚠️ mDNS start failed");
  } else {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("🌐 mDNS: http://%s.local\n", MDNS_HOSTNAME);
  }
}

void loop() {
  server.handleClient();
  if (restartPending && millis() > restartAtMs) {
    ESP.restart();
  }
  ElegantOTA.loop();
}
