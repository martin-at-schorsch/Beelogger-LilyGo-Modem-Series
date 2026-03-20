#include "web_server.h"
#include "config.h"
#include "sensors.h"
#include "modem.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#if ENABLE_DEBUG_WEB
AsyncWebServer server(80);

#include <nvs_flash.h>

WebCommand pendingCommand = CMD_NONE;
float pendingCalWeight = 0;
bool stayAwake = false;
unsigned long awakeTimerStart = 0;

String getProcessor(const String& var) {
    if (var == "TEMP") return String(currentTemperature, 1);
    if (var == "HUM") return String(currentHumidity, 1) + " &#37;";
    if (var == "PRES") return String(currentPressure, 1);
    if (var == "WEIGHT") return String(currentWeight, 2);
    if (var == "RAW_WEIGHT") return String(currentWeightRaw);
    if (var == "OFFSET") return String(currentLoadcellOffset);
    if (var == "DIVIDER") return String(currentLoadcellDivider, 2);
    if (var == "BAT") return String(currentBatteryVoltage, 2);
    if (var == "SOL") return String(currentSolarVoltage, 2);
    if (var == "VERSION") return String(APP_VERSION);
    
    if (var == "MODEM_INIT") {
        if (!sysStatus.modem_powered_on) return "Powered Off";
        return sysStatus.modem_init_success ? "OK" : "Error";
    }
    if (var == "MODEM_CLASS") {
        if (!sysStatus.modem_powered_on) return "badge-normal";
        return sysStatus.modem_init_success ? "badge-ok" : "badge-error";
    }
    
    if (var == "NET_CONN") {
        if (!sysStatus.modem_powered_on) return "N/A";
        return sysStatus.network_connected ? "Connected" : "Searching...";
    }
    if (var == "NET_TYPE") {
        if (!sysStatus.modem_powered_on) return "Offline";
        return sysStatus.network_type;
    }
    if (var == "MODEM_INFO") return sysStatus.modem_info;
    
    if (var == "SERV_CONN") {
        if (!sysStatus.modem_powered_on) return "N/A";
        return sysStatus.server_connected ? "Success" : "Pending/Fail";
    }
    if (var == "SERV_CLASS") {
        if (!sysStatus.modem_powered_on) return "badge-normal";
        return sysStatus.server_connected ? "badge-ok" : "badge-error";
    }
    
    if (var == "RSSI") {
        if (!sysStatus.modem_powered_on) return "---";
        return String(sysStatus.cell_rssi);
    }
    if (var == "UPTIME") {
        if (stayAwake) return String("ALWAYS-ON");
        long remaining = ACTIVE_AWAKE_TIMEOUT_SEC - ((millis() - awakeTimerStart) / 1000);
        if (remaining < 0) remaining = 0;
        return String(remaining) + " s";
    }
    if (var == "FREE_HEAP") return String(ESP.getFreeHeap() / 1024);
    
    if (var == "AWAKE_TEXT") return stayAwake ? "&#9728; ALWAYS-AWAKE" : "&#9632; NORMAL SLEEP";
    if (var == "AWAKE_CLASS") return stayAwake ? "badge-toggle toggle-on" : "badge-toggle toggle-off";
    if (var == "AWAKE_URL") return stayAwake ? "/awake_off" : "/awake_on";
    
    if (var == "PAYLOAD") {
        String p = sysStatus.last_payload;
        if (p.length() == 0) return "<i>No data transmitted yet...</i>";
        p.replace("&", "<br>");
        return p;
    }
    if (var == "RESPONSE") {
        if (sysStatus.last_server_response.length() == 0) return "<i>Waiting for first response...</i>";
        String r = sysStatus.last_server_response;
        r.replace("<", "&lt;"); r.replace(">", "&gt;"); 
        return r;
    }
    return String();
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Beelogger Pro | System Monitor</title>
  <style>
    :root {
      --primary: #fbbf24;
      --primary-hover: #f59e0b;
      --bg: #0f172a;
      --card: rgba(30, 41, 59, 0.7);
      --glass: rgba(255, 255, 255, 0.05);
      --text: #f8fafc;
      --text-muted: #94a3b8;
      --success: #10b981;
      --error: #ef4444;
      --awake: #8b5cf6;
      --awake-off: #64748b;
    }
    * { box-sizing: border-box; }
    body {
      font-family: -apple-system, system-ui, sans-serif;
      background: #0f172a;
      color: var(--text);
      margin: 0;
      padding: 0;
      min-height: 100vh;
    }
    header {
      width: 100vw;
      padding: 1.5rem;
      background: var(--glass);
      backdrop-filter: blur(10px);
      border-bottom: 1px solid var(--glass);
      display: flex;
      justify-content: space-between;
      align-items: center;
      position: sticky;
      top: 0;
      z-index: 100;
    }
    h1 { margin: 0; font-size: 1.1rem; letter-spacing: 0.1rem; }
    h1 b { color: var(--primary); }
    .version-badge { font-size: 0.6rem; background: var(--glass); padding: 2px 6px; border-radius: 4px; vertical-align: middle; margin-left: 8px; color: var(--text-muted); }
    
    .btn {
      padding: 0.5rem 1rem;
      border-radius: 0.5rem;
      text-decoration: none;
      font-size: 0.8rem;
      font-weight: 600;
      transition: all 0.2s;
      cursor: pointer;
      border: none;
      display: inline-block;
    }
    .btn-primary { background: var(--primary); color: #000; }
    .btn-primary:hover { background: var(--primary-hover); transform: translateY(-1px); }
    .btn-outline { border: 1px solid var(--glass); color: var(--text); background: var(--glass); }
    .btn-outline:hover { background: rgba(255,255,255,0.1); }
    
    .main-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
      gap: 1.25rem;
      max-width: 1200px;
      margin: 0 auto;
      padding: 1.5rem;
    }
    
    .card {
      background: var(--card);
      backdrop-filter: blur(12px);
      border: 1px solid var(--glass);
      border-radius: 1.25rem;
      padding: 1.5rem;
      box-shadow: 0 10px 15px -3px rgba(0,0,0,0.1);
    }
    
    .card-title {
      font-size: 0.75rem;
      text-transform: uppercase;
      color: var(--text-muted);
      margin-bottom: 1.25rem;
      display: flex;
      justify-content: space-between;
      letter-spacing: 0.05rem;
    }
    
    .hero-value {
      font-size: 2.75rem;
      font-weight: 800;
      color: var(--primary);
      text-align: center;
      margin: 0.5rem 0;
    }
    .hero-unit { font-size: 1rem; color: var(--text-muted); font-weight: 400; }
    
    .stat-row {
      display: flex;
      justify-content: space-between;
      padding: 0.6rem 0;
      border-bottom: 1px solid var(--glass);
    }
    .stat-label { color: var(--text-muted); font-size: 0.95rem; }
    .stat-value { font-weight: 600; }
    
    .badge {
      padding: 0.25rem 0.6rem;
      border-radius: 0.5rem;
      font-size: 0.75rem;
      font-weight: 700;
    }
    .badge-ok { background: rgba(16, 185, 129, 0.2); color: var(--success); }
    .badge-error { background: rgba(239, 68, 68, 0.2); color: var(--error); }
    
    .badge-toggle {
      padding: 0.35rem 0.75rem;
      border-radius: 2rem;
      font-size: 0.7rem;
      font-weight: 700;
      text-transform: uppercase;
      letter-spacing: 0.02rem;
      transition: all 0.2s ease;
      display: inline-flex;
      align-items: center;
      gap: 0.4rem;
      user-select: none;
      text-decoration: none !important;
    }
    .badge-toggle:hover { transform: scale(1.05); filter: brightness(1.2); }
    
    .toggle-on { 
      background: var(--awake); 
      color: white; 
      box-shadow: 0 0 15px rgba(139, 92, 246, 0.4);
    }
    .toggle-off { 
      background: rgba(255,255,255,0.1); 
      color: var(--text-muted); 
      border: 1px solid var(--glass);
    }
    
    .cal-controls {
      margin-top: 1rem;
      display: flex;
      flex-direction: column;
      gap: 0.75rem;
    }
    .input-group {
      display: flex;
      gap: 0.5rem;
    }
    input[type="number"] {
      flex: 1;
      background: rgba(0,0,0,0.3);
      border: 1px solid var(--glass);
      border-radius: 0.5rem;
      padding: 0.5rem;
      color: white;
      outline: none;
    }
    input[type="number"]:focus { border-color: var(--primary); }
    
    .data-box {
      background: rgba(3, 7, 18, 0.4);
      padding: 1rem;
      border-radius: 0.75rem;
      font-family: 'JetBrains Mono', 'Fira Code', monospace;
      font-size: 0.8rem;
      border-left: 4px solid var(--primary);
      word-break: break-all;
      line-height: 1.4;
    }
  </style>
</head>
<body>
  <header>
    <h1>BEELOGGER <b>PRO</b> <span class="version-badge">v%VERSION%</span></h1>
    <div style="display:flex; gap:0.5rem">
        <a href="/refresh" class="btn btn-outline" title="Refresh Sensors">&#8635;</a>
        <a href="/modem_send" class="btn btn-primary">SEND NOW</a>
    </div>
  </header>
  
  <div class="main-grid">
    <!-- NETWORK -->
    <div class="card">
      <div class="card-title">Cellular Status</div>
      <div class="stat-row">
        <span class="stat-label">Modem</span>
        <span class="badge %MODEM_CLASS%">%MODEM_INIT%</span>
      </div>
      <div class="stat-row">
        <span class="stat-label">Connection</span>
        <span class="stat-value" style="color:var(--primary)">%NET_CONN%</span>
      </div>
      <div class="stat-row">
        <span class="stat-label">Type</span>
        <span class="stat-value">%NET_TYPE%</span>
      </div>
      <div class="stat-row">
        <span class="stat-label">Signal</span>
        <span class="stat-value">%RSSI% dBm</span>
      </div>
      <div class="stat-row">
        <span class="stat-label">Data Backend</span>
        <span class="badge %SERV_CLASS%">%SERV_CONN%</span>
      </div>
      <div class="stat-row" style="border-bottom:none; margin-top:0.5rem">
        <span class="stat-label">Hardware Info</span>
        <span class="stat-value" style="font-size:0.75rem; color:var(--text-muted)">%MODEM_INFO%</span>
      </div>
    </div>

    <!-- SENSORS -->
    <div class="card">
      <div class="card-title">Sensor Readings</div>
      <div class="hero-value">%WEIGHT% <span class="hero-unit">kg</span></div>
      <div class="stat-row"><span>Temperature</span><span>%TEMP% &deg;C</span></div>
      <div class="stat-row"><span>Humidity</span><span>%HUM%</span></div>
      <div class="stat-row"><span>Pressure</span><span>%PRES% hPa</span></div>
      <div class="stat-row" style="border-top: 1px dotted var(--glass); margin-top:0.3rem">
        <span class="stat-label">RAW / Offset</span>
        <span class="stat-value" style="font-size:0.8rem; color:var(--text-muted)">%RAW_WEIGHT% / %OFFSET%</span>
      </div>
      <div class="stat-row">
        <span class="stat-label">Scale Divider</span>
        <span class="stat-value" style="font-size:0.8rem; color:var(--text-muted)">%DIVIDER%</span>
      </div>
    </div>

    <!-- MODEM CONTROL -->
    <div class="card">
      <div class="card-title">Modem Management</div>
      <div class="cal-controls">
        <div style="display:grid; grid-template-columns: 1fr 1fr; gap:0.5rem">
            <a href="/modem_on" class="btn btn-outline" style="text-align:center">BOOT</a>
            <a href="/modem_off" class="btn btn-outline" style="text-align:center">OFF</a>
            <a href="/modem_restart" class="btn btn-outline" style="text-align:center; grid-column: span 2">HARD RESTART</a>
        </div>
        <p style="font-size: 0.75rem; color: var(--text-muted); margin-top: 0.5rem; text-align:center">Control cellular hardware and power states.</p>
      </div>
    </div>

    <!-- CALIBRATION -->
    <div class="card">
      <div class="card-title">Scale Calibration</div>
      <div class="cal-controls">
        <p style="font-size: 0.85rem; color: var(--text-muted); margin: 0;">1. Empty scale and zero it:</p>
        <a href="/tare" class="btn btn-outline" style="text-align:center">TARE (ZERO SCALE)</a>
        
        <p style="font-size: 0.85rem; color: var(--text-muted); margin: 0.5rem 0 0 0;">2. Place known weight (g):</p>
        <div class="input-group">
          <input type="number" id="calWeight" placeholder="e.g. 500" value="500">
          <button onclick="calibrate()" class="btn btn-primary">CALIBRATE</button>
        </div>
        <div style="margin-top:0.8rem; text-align:right">
          <a href="/reset_cal" onclick="return confirm('Truly reset calibration to factory defaults?')" class="btn btn-outline" style="font-size:0.75rem; padding: 0.3rem 0.6rem; color:var(--error); border-color:var(--error)">CAL. RESET</a>
        </div>
      </div>
    </div>

    <!-- SYSTEM -->
    <div class="card">
      <div class="card-title">System & Power</div>
      <div style="margin-bottom:1rem">
        <a href="/reboot" onclick="return confirm('Reboot ESP32 system?')" class="btn" style="width:100&#37;; background:rgba(239, 68, 68, 0.1); border:1px solid var(--error); color:var(--error); text-align:center">ESP32 REBOOT</a>
      </div>
      <div class="stat-row"><span>Battery Volts</span><span>%BAT% V</span></div>
      <div class="stat-row"><span>Solar Volts</span><span>%SOL% V</span></div>
      <div class="stat-row"><span>Sleep In</span><span>%UPTIME%</span></div>
      <div class="stat-row"><span>Free Heap</span><span>%FREE_HEAP% KB</span></div>
      <div class="stat-row" style="border-top: 1px solid var(--glass); margin-top:0.5rem; padding-top:0.8rem">
        <span class="stat-label">Stay Awake Mode</span>
        <a href="%AWAKE_URL%" style="text-decoration:none"><span class="badge %AWAKE_CLASS%">%AWAKE_TEXT%</span></a>
      </div>
    </div>

    <!-- LOGS -->
    <div class="card" style="grid-column: 1 / -1;">
      <div class="card-title">Last Payload Transmitted</div>
      <div class="data-box">%PAYLOAD%</div>
      <div class="card-title" style="margin-top:1.5rem">Server Response</div>
      <div class="data-box" style="border-left-color: var(--success)">%RESPONSE%</div>
    </div>
  </div>

  <script>
    function calibrate() {
      const w = document.getElementById('calWeight').value;
      if (w > 0) {
        window.location.href = '/calibrate?weight=' + w;
      } else {
        alert('Please enter a valid weight in grams!');
      }
    }
    // Auto-refresh except when interacting with inputs
    setInterval(function(){ 
        if(document.activeElement.tagName !== 'INPUT') {
            window.location.reload(); 
        }
    }, 30000); 
  </script>
</body>
</html>
)rawliteral";

void startDebugWebServer() {
#if DEBUG_WIFI_USE_AP
    Serial.println("Starting Local Access Point (AP)...");
    WiFi.mode(WIFI_AP);
    if (WiFi.softAP(DEBUG_WIFI_SSID, DEBUG_WIFI_PASS)) {
        Serial.print("AP started. IP: "); Serial.println(WiFi.softAPIP());
    } else {
        Serial.println("AP startup FAILED!");
        return;
    }
#else
    Serial.println("Connecting to network WiFi Router...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(DEBUG_WIFI_SSID, DEBUG_WIFI_PASS);
    
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 15) {
        delay(500);
        Serial.print(".");
        retries++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi linked. IP: " + WiFi.localIP().toString());
    } else {
        Serial.println("\nWiFi link failed. Reverting to internal AP...");
        WiFi.mode(WIFI_AP);
        WiFi.softAP("Beelogger-Fallback", "12345678");
    }
#endif

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", index_html, getProcessor);
    });

    server.on("/tare", HTTP_GET, [](AsyncWebServerRequest *request){
        pendingCommand = CMD_TARE;
        request->redirect("/");
    });

    server.on("/calibrate", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("weight")) {
            pendingCalWeight = request->getParam("weight")->value().toFloat();
            pendingCommand = CMD_CALIBRATE;
        }
        request->redirect("/");
    });

    server.on("/refresh", HTTP_GET, [](AsyncWebServerRequest *request){
        pendingCommand = CMD_REFRESH;
        request->redirect("/");
    });

    server.on("/modem_send", HTTP_GET, [](AsyncWebServerRequest *request){
        pendingCommand = CMD_MODEM_SEND;
        request->redirect("/");
    });

    server.on("/modem_on", HTTP_GET, [](AsyncWebServerRequest *request){
        pendingCommand = CMD_MODEM_ON;
        request->redirect("/");
    });

    server.on("/modem_off", HTTP_GET, [](AsyncWebServerRequest *request){
        pendingCommand = CMD_MODEM_OFF;
        request->redirect("/");
    });

    server.on("/modem_restart", HTTP_GET, [](AsyncWebServerRequest *request){
        pendingCommand = CMD_MODEM_RESTART;
        request->redirect("/");
    });

    server.on("/awake_on", HTTP_GET, [](AsyncWebServerRequest *request){
        stayAwake = true;
        pendingCommand = CMD_STAY_AWAKE_ON;
        request->redirect("/");
    });

    server.on("/awake_off", HTTP_GET, [](AsyncWebServerRequest *request){
        stayAwake = false;
        pendingCommand = CMD_STAY_AWAKE_OFF;
        request->redirect("/");
    });

    server.on("/reset_cal", HTTP_GET, [](AsyncWebServerRequest *request){
        pendingCommand = CMD_RESET_CAL;
        request->redirect("/");
    });

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request){
        pendingCommand = CMD_REBOOT;
        request->send(200, "text/plain", "ESP32 is rebooting... Please reload page in 10 seconds.");
    });

    server.begin();
    Serial.println("Debug Web UI listening.");
}

void handleWebCommands() {
    if (pendingCommand == CMD_NONE) return;

    WebCommand cmd = pendingCommand;
    pendingCommand = CMD_NONE; 

    Serial.print("Processing Web UI Command: ");
    Serial.println(cmd);

    switch (cmd) {
        case CMD_TARE:
            tareScale();
            break;
        case CMD_CALIBRATE:
            calibrateScale(pendingCalWeight);
            break;
        case CMD_REFRESH:
            readSensors();
            break;
        case CMD_MODEM_ON:
            setupModem();
            connectNetwork();
            break;
        case CMD_MODEM_OFF:
            powerDownModem();
            break;
        case CMD_MODEM_RESTART:
            powerDownModem();
            delay(1000);
            setupModem();
            delay(500); 
            connectNetwork();
            break;
        case CMD_MODEM_SEND:
            readSensors();
            {
                String payload = "T1=" + String(currentTemperature, 1) + 
                                 "&F1=" + String(currentHumidity, 1) +
                                 "&A1=" + String(currentPressure, 1) +
                                 "&G1=" + String(currentWeight, 2) +
                                 "&VB=" + String(currentBatteryVoltage, 2) +
                                 "&VS=" + String(currentSolarVoltage, 2);
                float checkSumVal = currentTemperature + currentHumidity + currentPressure + currentWeight + currentBatteryVoltage + currentSolarVoltage;
                payload += "&C=" + String((int)round(checkSumVal + 0.5));
                payload += "&PW=beelogger";
                
                if (!sysStatus.modem_init_success) {
                    setupModem();
                }
                if (connectNetwork()) {
                    postData(payload);
                }
                if (!stayAwake) powerDownModem();
            }
            break;
        case CMD_STAY_AWAKE_ON:
            stayAwake = true;
            break;
        case CMD_STAY_AWAKE_OFF:
            stayAwake = false;
            break;
        case CMD_RESET_CAL:
            resetScaleCalibration();
            break;
        case CMD_REBOOT:
            Serial.println("Executing system reboot...");
            delay(500);
            ESP.restart();
            break;
        default:
            break;
    }
}

void handleDebugWebServer() {}
bool isWebDebugEnabled() { return true; }

#else
void startDebugWebServer() {}
void handleDebugWebServer() {}
bool isWebDebugEnabled() { return false; }
#endif
