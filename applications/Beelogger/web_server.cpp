#include "web_server.h"
#include "config.h"
#include "sensors.h"
#include "modem.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#if ENABLE_WORK_ON_HIVE_WEB
AsyncWebServer server(80);

#include <nvs_flash.h>
#include <Update.h>

WebCommand pendingCommand = CMD_NONE;
float pendingCalWeight = 0;
bool stayAwake = false;
bool workModeActive = false;
bool serverStarted = false;
unsigned long awakeTimerStart = 0;
uint32_t activeTimeout = 0;

String getProcessor(const String& var) {
    if (var == "V_T") return String(currentTemperature, 1);
    if (var == "V_H") return String(currentHumidity, 1);
    if (var == "V_P") return String(currentPressure, 1);
    if (var == "V_W") return String(currentWeight, 2);
    if (var == "V_B") return String(currentBatteryVoltage, 2);
    if (var == "V_S") return String(currentSolarVoltage, 2);
    if (var == "V_VER") return String(APP_VERSION);
    if (var == "V_RAW") return String(currentWeightRaw);
    if (var == "V_OFF") return String(currentLoadcellOffset);
    if (var == "V_DIV") return String(currentLoadcellDivider, 2);
    if (var == "V_FREE") return String(ESP.getFreeHeap() / 1024);
    if (var == "V_WR") return String(WiFi.RSSI());
    
    if (var == "V_ST") {
        if (stayAwake) return "ALWAYS-AWAKE";
        long rem = activeTimeout - ((millis() - awakeTimerStart) / 1000);
        return String(rem > 0 ? rem : 0) + "s";
    }

    if (var == "V_MI") {
        if (!sysStatus.modem_powered_on) return "Powerdown";
        return sysStatus.modem_init_success ? "OK" : "Error";
    }
    if (var == "V_MC") {
        if (!sysStatus.modem_powered_on) return "";
        return sysStatus.modem_init_success ? "badge-ok" : "badge-error";
    }
    if (var == "V_NT") return sysStatus.modem_powered_on ? sysStatus.network_type : "Offline";
    if (var == "V_NC") return sysStatus.modem_powered_on ? (sysStatus.network_connected ? "Connected" : "Searching...") : "Off";
    if (var == "V_NC_C") return sysStatus.network_connected ? "color:var(--success)" : "color:#fbbf24";
    if (var == "V_RSSI") return sysStatus.modem_powered_on ? String(sysStatus.cell_rssi, 2) + " dBm" : "N/A";
    
    if (var == "V_UP") {
        unsigned long s = millis() / 1000;
        unsigned long m = s / 60;
        unsigned long h = m / 60;
        return String(h) + "h " + String(m % 60) + "m " + String(s % 60) + "s";
    }
    
    if (var == "V_PAY") {
        String p = sysStatus.last_payload;
        if (p.length() == 0) return "No data yet";
        return p;
    }
    if (var == "V_PT") {
        if (sysStatus.last_payload_time == 0) return "";
        return "(Sent " + String((millis() - sysStatus.last_payload_time)/1000) + " seconds ago)";
    }
    if (var == "V_RES") return sysStatus.last_server_response.length() > 0 ? sysStatus.last_server_response : "Waiting/Failed to connect";
    if (var == "V_HI") return sysStatus.modem_info;
    
    return "";
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Beelogger ESP32</title>
<style>
  :root { --bg:#111624; --card:#1c243a; --glass:rgba(255,255,255,0.08); --text:#e5e7eb; --text-muted:#9ca3af; --primary:#fbbf24; --success:#10b981; --error:#ef4444; --awake:#8b5cf6; }
  body { font-family:sans-serif; background:var(--bg); color:var(--text); margin:0; line-height:1.4; }
  header { background:rgba(0,0,0,0.3); padding:0.75rem 1.25rem; border-bottom:1px solid var(--glass); display:flex; justify-content:space-between; align-items:center; }
  .btn { padding:0.5rem 0.8rem; border-radius:0.4rem; text-decoration:none; font-size:0.75rem; font-weight:700; cursor:pointer; border:1px solid var(--glass); text-align:center; background:var(--glass); color:var(--text); box-sizing:border-box; }
  .btn:hover { background:rgba(255,255,255,0.12); }
  .btn-p { background:var(--primary); color:#000; border:none; }
  .btn-danger { background:rgba(239,68,68,0.15); border:1px solid var(--error); color:var(--error); }
  .badge { padding:0.15rem 0.4rem; border-radius:0.3rem; font-size:0.65rem; font-weight:700; }
  .badge-ok { background:rgba(16,185,129,0.2); color:var(--success); border:1px solid var(--success); }
  .badge-error { background:rgba(239,68,68,0.2); color:var(--error); border:1px solid var(--error); }
  .grid { display:grid; grid-template-columns:repeat(auto-fit, minmax(300px, 1fr)); gap:1.25rem; padding:1.25rem; max-width:1400px; margin:0 auto; }
  .card { background:var(--card); border:1px solid var(--glass); border-radius:1rem; padding:1.25rem; position:relative; }
  .card-h { font-size:0.65rem; text-transform:uppercase; color:var(--text-muted); margin-bottom:1rem; border-bottom:1px solid var(--glass); padding-bottom:0.3rem; font-weight:700; }
  .hero { font-size:3rem; font-weight:900; color:var(--primary); text-align:center; margin:0.75rem 0; }
  .row { display:flex; justify-content:space-between; padding:0.4rem 0; border-bottom:1px solid var(--glass); font-size:0.85rem; }
  .box { background:rgba(0,0,0,0.4); padding:0.75rem; border-radius:0.5rem; font-family:monospace; font-size:0.75rem; border:1px solid var(--glass); white-space:pre-wrap; color:#d1d5db; max-height:200px; overflow-y:auto; }
  input[type=number], input[type=file] { background:#000; color:#fff; border:1px solid var(--glass); padding:0.4rem; border-radius:0.3rem; font-size:0.8rem; }
  hr { border:0; border-top:1px dashed var(--glass); margin:1rem 0; }
</style>
</head><body>
<header>
  <div><small style="opacity:0.6;font-size:0.5rem">WORK-ON-HIVE INTERFACE</small><br><span style="font-weight:700">BEELOGGER ESP32</span> <small style="opacity:0.3;font-size:0.5rem">v%V_VER%</small></div>
  <div style="display:flex; gap:0.5rem">
    <a href="/refresh" class="btn">&#8635; REFRESH</a>
    <a href="/modem_send" class="btn btn-p">SEND NOW</a>
  </div>
</header>
<div class="grid">
  <div class="card" style="grid-column: 1 / -1; border-color: var(--awake)">
    <div style="display:flex; justify-content:space-between; align-items:center">
      <div><div style="font-size:1.15rem;font-weight:700">Maintenance Active</div><small style="color:var(--text-muted)">The device stays awake to allow debugging and calibration.</small></div>
      <div style="text-align:right">
        Sleep In: <b> %V_ST% </b> <br>
        <a href="/awake_on" class="btn" style="padding:0.25rem 0.5rem;font-size:0.6rem;background:var(--glass);color:#fff">&#9632; NORMAL SLEEP</a>
      </div>
    </div>
  </div>

  <div class="card">
    <div class="card-h">SENSOR READINGS</div>
    <div class="hero"> %V_W% <small style="font-size:1.25rem;opacity:0.5;font-weight:400">kg</small></div>
    <div class="row"><span>Temperature</span><span> %V_T% &deg;C</span></div>
    <div class="row"><span>Humidity</span><span> %V_H% &#37;</span></div>
    <div class="row"><span>Pressure</span><span> %V_P% hPa</span></div>
    <div class="row"><span>Battery Volts</span><span> %V_B% V</span></div>
    <div class="row"><span>Solar Volts</span><span> %V_S% V</span></div>
  </div>

  <div class="card">
    <div class="card-h">SCALE MAINTENANCE</div>
    <div style="margin-bottom:1rem">
      <div style="font-size:0.75rem;margin-bottom:0.4rem">1. Empty scale and zero it:</div>
      <a href="/tare" class="btn" style="width:100&#37;">TARE (ZERO SCALE)</a>
    </div>
    <div style="margin-top:1rem">
      <div style="font-size:0.75rem;margin-bottom:0.4rem">2. Place known weight (kg):</div>
      <div style="display:flex; gap:0.4rem">
        <input type="number" id="cw" value="1.0" step="0.1" style="flex:1">
        <button onclick="window.location.href='/calibrate?weight='+document.getElementById('cw').value" class="btn btn-p" style="flex:1">CALIBRATE</button>
      </div>
      <div style="display:flex; justify-content:flex-end; margin-top:0.75rem">
        <a href="/tare_reset" class="btn btn-danger" style="font-size:0.6rem;padding:0.25rem 0.6rem">CAL. RESET</a>
      </div>
    </div>
  </div>

  <div class="card">
    <div class="card-h">CELLULAR & MODEM</div>
    <div class="row"><span>Modem Power</span><span class="badge %V_MC%"> %V_MI% </span></div>
    <div class="row"><span>Connection</span><span style="%V_NC_C%"> %V_NC% </span></div>
    <div class="row"><span>Network Type</span><span> %V_NT% </span></div>
    <div class="row"><span>Signal</span><span> %V_RSSI% </span></div>
    <div class="row"><span>Data Backend</span><span style="color:var(--error)">Pending/Fail</span></div>
    <hr>
    <div style="font-size:0.65rem; color:var(--text-muted); margin-bottom:0.5rem">Modem Controls</div>
    <div style="display:grid; grid-template-columns:1fr 1fr; gap:0.5rem">
      <a href="/modem_on" class="btn">BOOT</a>
      <a href="/modem_off" class="btn">OFF</a>
      <a href="/modem_restart" class="btn" style="grid-column:1/3">HARD RESTART</a>
    </div>
  </div>

  <div class="card">
    <div class="card-h">FIRMWARE UPDATE</div>
    <div style="margin-bottom:1rem">
       <div style="font-size:0.75rem;margin-bottom:0.4rem">Check cloud for updates:</div>
       <div style="display:grid; grid-template-columns:1fr 1fr; gap:0.5rem">
         <a href="/ota_modem" class="btn" style="font-size:0.6rem">OTA (MOBILE)</a>
         <a href="/ota_wifi" class="btn" style="font-size:0.6rem">OTA (WIFI)</a>
       </div>
    </div>
    <hr>
    <div>
       <div style="font-size:0.75rem;margin-bottom:0.4rem">Or upload file manually:</div>
       <form method='POST' action='/update' enctype='multipart/form-data'>
          <input type='file' name='update' style="width:100&#37;;font-size:0.7rem;margin-bottom:0.4rem">
          <input type='submit' value='Upload & Flash' class="btn btn-p" style="width:100&#37;">
       </form>
    </div>
    <div style="margin-top:1rem">
      <a href="/reboot" class="btn btn-danger" style="width:100&#37;">REBOOT SYSTEM</a>
    </div>
  </div>

  <div class="card" style="grid-column: 1 / -1">
    <div class="card-h">LAST PAYLOAD TRANSMITTED <span style="float:right;text-transform:none"> %V_PT% </span></div>
    <div class="box"> %V_PAY% </div>
  </div>

  <div class="card" style="grid-column: 1 / -1">
    <div class="card-h">SERVER RESPONSE <a href="/clear_log" style="float:right; color:var(--text-muted); text-decoration:none; font-size:0.6rem; border:1px solid var(--glass); padding:0 0.3rem; border-radius:0.2rem">CLEAR</a></div>
    <div class="box" style="border-left:3px solid var(--error)"> %V_RES% </div>
  </div>

  <div class="card" style="grid-column: 1 / -1">
    <div class="card-h">DEBUG INFORMATION</div>
    <div style="display:grid; grid-template-columns:repeat(auto-fit, minmax(200px, 1fr)); gap:0.75rem; font-size:0.75rem">
      <div><span style="color:var(--text-muted)">HX711 RAW Value</span><br><b> %V_RAW% </b></div>
      <div><span style="color:var(--text-muted)">Scale Offset</span><br><b> %V_OFF% </b></div>
      <div><span style="color:var(--text-muted)">Scale Factor (Divider)</span><br><b> %V_DIV% </b></div>
      <div><span style="color:var(--text-muted)">WIFI RSSI</span><br><b> %V_WR% dBm</b></div>
      <div><span style="color:var(--text-muted)">ESP32 Free Heap</span><br><b> %V_FREE% KB</b></div>
    </div>
  </div>

  <div class="card" style="grid-column: 1 / -1; background:rgba(0,0,0,0.15)">
    <div class="card-h">CELLULAR HARDWARE INFO</div>
    <div class="box" style="font-size:0.7rem;line-height:1.2"> %V_HI% </div>
  </div>
</div>
<script>
  setInterval(async function(){
    if(document.activeElement.tagName==='INPUT') return;
    try {
      const r=await fetch(window.location.href);
      const t=await r.text();
      const doc=new DOMParser().parseFromString(t,'text/html');
      const grid=doc.querySelector('.grid');
      if(grid) document.querySelector('.grid').innerHTML=grid.innerHTML;
    } catch(e){}
  }, 5000);
</script>
</body></html>
)rawliteral";

void startWorkModeWebServer() {
    if (serverStarted) return;
    
    sysStatus.last_server_response = ""; // Reset to prevent boot-up logic spam appearing in UI
    
    // Static HTML Index Route
#if WIFI_WORK_MODE_AP
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
#else
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASS);
#endif

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", index_html, getProcessor);
    });

    server.on("/ota_modem", HTTP_GET, [](AsyncWebServerRequest *request){
        pendingCommand = CMD_OTA_MODEM;
        request->redirect("/");
    });

    server.on("/ota_wifi", HTTP_GET, [](AsyncWebServerRequest *request){
        pendingCommand = CMD_OTA_WIFI;
        request->redirect("/");
    });

    server.on("/tare", HTTP_GET, [](AsyncWebServerRequest *request){
        pendingCommand = CMD_TARE;
        request->redirect("/");
    });

    server.on("/tare_reset", HTTP_GET, [](AsyncWebServerRequest *request){
        pendingCommand = CMD_TARE_RESET;
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
        readSensors();
        pendingCommand = CMD_REFRESH;
        request->redirect("/");
    });

    server.on("/modem_send", HTTP_GET, [](AsyncWebServerRequest *request){
        pendingCommand = CMD_MODEM_SEND;
        request->redirect("/");
    });

    server.on("/clear_log", HTTP_GET, [](AsyncWebServerRequest *request){
        sysStatus.last_server_response = "";
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
        stayAwake = !stayAwake; // Toggle functionality like in original
        request->redirect("/");
    });

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request){
        pendingCommand = CMD_REBOOT;
        request->send(200, "text/plain", "Rebooting device...");
    });

    server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", (Update.hasError()) ? "Update Failed" : "Update Success. Rebooting...");
        delay(1000);
        ESP.restart();
    }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
        if (!index) {
          if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { Update.printError(Serial); }
        }
        if (!Update.hasError()) {
          if (Update.write(data, len) != len) { Update.printError(Serial); }
        }
        if (final) {
          if (!Update.end(true)) { Update.printError(Serial); }
        }
    });

    server.begin();
    serverStarted = true;
}

void handleWebCommands() {
    if (pendingCommand == CMD_NONE) return;
    WebCommand cmd = pendingCommand;
    pendingCommand = CMD_NONE; 
    awakeTimerStart = millis();

    switch (cmd) {
        case CMD_TARE: tareScale(); break;
        case CMD_TARE_RESET: {
            extern void resetScaleCalibration();
            resetScaleCalibration();
        } break;
        case CMD_CALIBRATE: calibrateScale(pendingCalWeight); break;
        case CMD_REFRESH: readSensors(); break;
        case CMD_MODEM_SEND:
            readSensors();
            {
                String payload = "T1=" + String(currentTemperature, 1) + 
                                 "&F1=" + String(currentHumidity, 1) +
                                 "&A1=" + String(currentPressure, 1) + 
                                 "&G1=" + String(currentWeight, 2) +
                                 "&VB=" + String(currentBatteryVoltage, 2) +
                                 "&VS=" + String(currentSolarVoltage, 2) +
                                 "&PW=beelogger";
                sysStatus.last_payload = payload;
                sysStatus.last_payload_time = millis();
                if (connectNetwork()) postData(payload);
            }
            break;
        case CMD_MODEM_ON: setupModem(); break;
        case CMD_MODEM_OFF: powerDownModem(); break;
        case CMD_MODEM_RESTART:
            powerDownModem();
            delay(500);
            setupModem();
            connectNetwork();
            break;
        case CMD_REBOOT: ESP.restart(); break;
        case CMD_OTA_MODEM: {
                performModemOTA(OTA_MANIFEST_URL);
            }
            break;
        case CMD_OTA_WIFI: {
                performWifiOTA(OTA_MANIFEST_URL);
            }
            break;
        default: break;
    }
}

bool isWorkModeEnabled() { return true; }
#else
void startWorkModeWebServer() {}
void handleWebCommands() {}
bool isWorkModeEnabled() { return false; }
#endif
