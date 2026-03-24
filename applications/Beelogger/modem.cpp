#include <WiFi.h>
#include <WiFiClientSecure.h> 
#include "modem.h"
#include "config.h"

// Modem selection and SerialAT definition is now handled by utilities.h
#include <TinyGsmClient.h>
#include <ArduinoJson.h>
#include <Update.h>
#include "sensors.h"

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);

SystemStatus sysStatus;


void setupModem() {
    pinMode(MODEM_POWERON_PIN, OUTPUT);
    digitalWrite(MODEM_POWERON_PIN, HIGH); // Modem power switch
    sysStatus.modem_powered_on = true;

    pinMode(MODEM_PWRKEY_PIN, OUTPUT);
    pinMode(MODEM_RESET_PIN, OUTPUT);
    digitalWrite(MODEM_RESET_PIN, LOW);

    SerialAT.begin(MODEM_BAUDRATE, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
    
    // Check if modem is already responding
    Serial.println("Verifying if modem is already awake...");
    delay(100);
    SerialAT.println("AT");
    unsigned long start = millis();
    bool alive = false;
    while(millis() - start < 1000) {
        if(SerialAT.available()) {
            String r = SerialAT.readString();
            if(r.indexOf("OK") >= 0) {
                alive = true;
                break;
            }
        }
    }

    if (!alive) {
        Serial.println("Modem initialization (Pulse PWRKEY)...");
        digitalWrite(MODEM_PWRKEY_PIN, LOW);
        delay(100);
        digitalWrite(MODEM_PWRKEY_PIN, HIGH);
        delay(MODEM_POWERON_PULSE_WIDTH_MS); // Pulse duration based on board
        digitalWrite(MODEM_PWRKEY_PIN, LOW);
        delay(MODEM_START_WAIT_MS); // Wait for bootup
    } else {
        Serial.println("Modem is already active.");
    }
}

bool connectNetwork() {
    Serial.println("Initializing modem (up to 30s)...");
    if (!modem.init()) {
        Serial.println("Init failed. Forcing complete restart...");
        if (!modem.restart()) {
            Serial.println("Critical error: Failed to restart modem!");
            sysStatus.modem_init_success = false;
            return false;
        }
    }
    sysStatus.modem_init_success = true;

    // Retrieve Hardware Info
    sysStatus.modem_info = "";
    SerialAT.println("AT+SIMCOMATI");
    int lineCount = 0; 
    unsigned long infoTimeout = millis();
    while (millis() - infoTimeout < 2000 && lineCount < 10) {
        if (SerialAT.available()) {
            String line = SerialAT.readStringUntil('\n');
            line.trim();
            if (line.length() > 0 && line != "OK" && line != "AT+SIMCOMATI") {
                if (sysStatus.modem_info.length() > 0) sysStatus.modem_info += "<br>";
                sysStatus.modem_info += line;
                lineCount++;
            }
            if (line == "OK") break;
            infoTimeout = millis();
        }
    }
    if (sysStatus.modem_info.length() == 0) sysStatus.modem_info = modem.getModemName();

    // Configure preferred Network Mode
    Serial.println("Configuring carrier preferences (Auto LTE/GSM)...");
    SerialAT.println("AT+CNMP=2"); // 2 = Auto, 38 = LTE-Only
    modem.waitResponse(1000);

    Serial.print("Searching for cellular network...");
    if (workModeActive) sysStatus.last_server_response = "MODEM: Searching for cellular network...\n" + sysStatus.last_server_response;
    if (!modem.waitForNetwork(60000L)) {
        Serial.println(" timeout/fail");
        if (workModeActive) sysStatus.last_server_response = "MODEM: Network search timeout/fail\n" + sysStatus.last_server_response;
        sysStatus.network_connected = false;
        return false;
    }
    Serial.println(" successful");
    if (workModeActive) sysStatus.last_server_response = "MODEM: Network found!\n" + sysStatus.last_server_response;
    sysStatus.cell_rssi = modem.getSignalQuality();

    // Log Network Mode (LTE/GPRS)
    SerialAT.println("AT+CNSMOD?");
    if (modem.waitResponse(1000, "+CNSMOD: ") == 1) {
        String res = SerialAT.readStringUntil('\n');
        int mode = res.substring(res.indexOf(',') + 1).toInt();
        switch(mode) {
            case 1: sysStatus.network_type = "GPRS (2G)"; break;
            case 2: sysStatus.network_type = "EDGE (2.5G)"; break;
            case 8: sysStatus.network_type = "LTE (4G)"; break;
            case 12: sysStatus.network_type = "LTE (4G)"; break;
            default: sysStatus.network_type = "Mode " + String(mode); break;
        }
        Serial.print("Network Connection Type: ");
        Serial.println(sysStatus.network_type);
        if (workModeActive) sysStatus.last_server_response = "MODEM: Connection Type " + sysStatus.network_type + "\n" + sysStatus.last_server_response;
    }

    Serial.print("Connecting to GPRS Data Service... ");
    sysStatus.network_connected = false; 
    bool dataConnected = false;
    for(int i = 0; i < 3; i++) {
        if (modem.gprsConnect(APN, GPRS_USER, GPRS_PASS)) {
            dataConnected = true;
            break;
        }
        Serial.print("failed (retry)... ");
        delay(3000);
    }

    if (!dataConnected) {
        Serial.println("failed permanently");
        if (workModeActive) sysStatus.last_server_response = "MODEM: GPRS Data connection failed\n" + sysStatus.last_server_response;
        sysStatus.network_connected = false;
        return false;
    }
    Serial.println("connected");
    if (workModeActive) sysStatus.last_server_response = "MODEM: GPRS Data connected.\n" + sysStatus.last_server_response;
    sysStatus.network_connected = true;
    return true;
}

bool postData(String payload) {
    // Sanitize values
    payload.replace("nan", "0.00");
    
    sysStatus.last_payload = payload;
    sysStatus.last_payload_time = millis();
    if (!client.connect(SERVER_HOST, SERVER_PORT)) {
        Serial.println("TCP Connection to backend failed!");
        sysStatus.server_connected = false;
        return false;
    }
    sysStatus.server_connected = true;

    Serial.println("Backend connected. Transmitting GET payload...");
    
    // Transmit GET request (standard Beelogger protocol format)
    client.print(String("GET ") + SERVER_PATH + "?" + payload + " HTTP/1.1\r\n");
    client.print(String("Host: ") + SERVER_HOST + "\r\n");
    client.print("Connection: close\r\n");
    client.print("\r\n");

    unsigned long start_wait = millis();
    String response = "";
    Serial.println("Waiting for response...");
    
    // Most LTE modems need a moment to swap buffers
    while (millis() - start_wait < 15000L) {
        while (client.available()) {
            char c = client.read();
            response += c;
            Serial.print(c);
            start_wait = millis(); // Reset and keep reading
        }
        if (!client.connected()) {
            // Give it one last chance to report trailing data
            delay(200);
            if (!client.available()) break;
        }
        delay(10);
    }
    client.stop();
    
    if (response.length() == 0) {
        if (workModeActive) sysStatus.last_server_response = "Server connection closed (No data returned)\n" + sysStatus.last_server_response;
    } else {
        if (workModeActive) sysStatus.last_server_response = response + "\n" + sysStatus.last_server_response;
        
        // Extract timestamp if present: (TIMESTAMP)ok *
        int firstDigit = -1;
        for(int i=0; i<response.length(); i++) {
            if(isdigit(response[i])) { firstDigit = i; break; }
        }
        if (firstDigit >= 0) {
            sysStatus.last_server_timestamp = strtoul(response.c_str() + firstDigit, NULL, 10);
            if (sysStatus.last_server_timestamp > 0) {
                Serial.printf("Extracted Server Timestamp: %u\n", sysStatus.last_server_timestamp);
            }
        }
    }
    
    Serial.println("\nRequest cycle completed.");
    return true;
}

void powerDownModem() {
    Serial.println("Shutting down Modem power-down...");
    modem.poweroff();
    
    // Shut off the modem initialization pin if different from system power
    if (MODEM_POWERON_PIN != SYSTEM_POWER_PIN) {
        digitalWrite(MODEM_POWERON_PIN, LOW);
    }
    
    // Reset runtime status
    sysStatus.modem_powered_on = false;
    sysStatus.modem_init_success = false;
    sysStatus.network_connected = false;
    sysStatus.server_connected = false;
    sysStatus.network_type = "Offline";
    sysStatus.cell_rssi = -115;
    // Don't clear modem_info so IMEI and Hardware Specs are retained across boots
}

// Internal helper for the actual download and flash
bool performOTA(const char* url, size_t size) {
    if (size == 0) return false;
    
    Serial.printf("Starting OTA from: %s (%d bytes)\n", url, size);
    
    if (!Update.begin(size)) {
        Serial.println("OTA Error: Not enough space.");
        return false;
    }

    if (!modem.https_set_url(url)) {
        Serial.println("OTA Error: Failed to set binary URL.");
        Update.end();
        return false;
    }

    size_t firmware_size = 0;
    int httpCode = modem.https_get(&firmware_size);
    if (httpCode != 200) {
        Serial.printf("OTA Error: HTTP GET Failed (%d)\n", httpCode);
        Update.end();
        return false;
    }

    uint8_t buffer[1024];
    int total = 0;
    while (true) {
        int len = modem.https_body(buffer, 1024);
        if (len <= 0) break;
        if (Update.write(buffer, len) != len) {
            Serial.println("OTA Error: Write failed.");
            sysStatus.last_server_response = "OTA Error: Flash write failed";
            break;
        }
        total += len;
        int progress = (total * 100) / (int)size;
        Serial.printf("\rProgress: %d%%", progress);
        
        // Update web UI status every 2.5% to avoid too many updates but keep it fresh
        static int lastProg = -1;
        if (progress / 2 != lastProg) {
            String msg = "Downloading Update: " + String(progress) + "&#37; (" + String(total/1024) + " KB)";
            sysStatus.last_server_response = msg + "\n" + sysStatus.last_server_response;
            if (sysStatus.last_server_response.length() > 2048) sysStatus.last_server_response = sysStatus.last_server_response.substring(0, 2048);
            lastProg = progress / 2;
        }
    }

    if (Update.end(true)) {
        Serial.println("\nOTA Success! Rebooting...");
        sysStatus.last_server_response = "OTA Success! System is rebooting...";
        delay(3000); // Give user time to see message
        esp_restart();
        return true;
    } else {
        Serial.printf("\nOTA Error: #%d\n", Update.getError());
        sysStatus.last_server_response = "OTA Error: Flash finalization failed (Code " + String(Update.getError()) + ")";
        return false;
    }
}

bool performModemOTA(const char* manifestUrl) {
    sysStatus.last_server_response = "OTA-MODEM: Starting search...\n" + sysStatus.last_server_response;
    
    if (!sysStatus.network_connected) {
        sysStatus.last_server_response = "Powering up modem for update...\n" + sysStatus.last_server_response;
        Serial.println("OTA: Modem not connected. Attempting auto-connect...");
        setupModem();
        if (!connectNetwork()) {
            sysStatus.last_server_response = "OTA Error: Failed to connect to LTE network.\n" + sysStatus.last_server_response;
            return false;
        }
    }

    sysStatus.last_server_response = "Fetching version manifest...\n" + sysStatus.last_server_response;
    Serial.println("Fetching: " + String(manifestUrl));
    
#ifdef LILYGO_T_A7670
    const char* myHw = "T-A7670X";
#else
    const char* myHw = "Generic";
#endif

    modem.https_begin();
    if (!modem.https_set_url(manifestUrl)) {
        sysStatus.last_server_response = "OTA Error: Invalid manifest URL.";
        modem.https_end();
        return false;
    }

    size_t bodySize = 0;
    int httpCode = modem.https_get(&bodySize);
    if (httpCode != 200 || bodySize == 0) {
        sysStatus.last_server_response = "OTA Error: Manifest fetch failed (HTTP " + String(httpCode) + ")";
        modem.https_end();
        return false;
    }

    char* manifestBuffer = (char*)malloc(bodySize + 1);
    if (!manifestBuffer) {
        sysStatus.last_server_response = "OTA Error: Out of memory (JSON)";
        modem.https_end();
        return false;
    }

    int readLen = modem.https_body((uint8_t*)manifestBuffer, bodySize);
    manifestBuffer[readLen] = '\0';
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, manifestBuffer);
    free(manifestBuffer);

    if (error) {
        sysStatus.last_server_response = "OTA Error: JSON Parse Failed";
        modem.https_end();
        return false;
    }

    // New logic: Look for hardware-specific block
    if (!doc[myHw].is<JsonObject>()) {
        sysStatus.last_server_response = "OTA Info: No entry for hardware '" + String(myHw) + "'\n" + sysStatus.last_server_response;
        modem.https_end();
        return false;
    }

    JsonObject entry = doc[myHw];
    const char* newVersion = entry["version"];
    const char* binaryUrl = entry["url"];
    size_t binSize = entry["size"] | 0;

    if (!newVersion || !binaryUrl) {
        sysStatus.last_server_response = "OTA Error: Malformed manifest for " + String(myHw);
        modem.https_end();
        return false;
    }

    Serial.print("Current Version: "); Serial.println(APP_VERSION);
    Serial.print("Cloud Version:   "); Serial.println(newVersion);

    // Version Comparison (simple string compare for now, or semantic)
    if (String(newVersion) == String(APP_VERSION)) {
        sysStatus.last_server_response = "System up to date (Version " + String(APP_VERSION) + ")\n" + sysStatus.last_server_response;
        modem.https_end();
        return false;
    }

    sysStatus.last_server_response = "Update found! Downloading v" + String(newVersion) + " (" + String(binSize/1024) + " KB)...\n" + sysStatus.last_server_response;
    
    // Proceed to download binary
    bool success = performOTA(binaryUrl, binSize);
    
    modem.https_end();
    return success;
}

bool performWifiOTA(const char* manifestUrl) {
    sysStatus.last_server_response = "OTA-WIFI: Starting search...\n" + sysStatus.last_server_response;
    
    if (WiFi.status() != WL_CONNECTED) {
        sysStatus.last_server_response = "OTA-WIFI Error: WiFi not connected!\n" + sysStatus.last_server_response;
        return false;
    }

    // Since we avoid HTTPClient, we use WiFiClientSecure directly
    WiFiClientSecure sclient;
    sclient.setInsecure();

    // 1. Fetch Manifest
    String host = "nx.msdt.at";
    String path = "/public.php/dav/files/PckBJ7cnmQj7KmM/version.json";
    
    sysStatus.last_server_response = "OTA-WIFI: Connecting to host...\n" + sysStatus.last_server_response;
    if (!sclient.connect(host.c_str(), 443)) {
        sysStatus.last_server_response = "OTA-WIFI Error: Connection to host failed.\n" + sysStatus.last_server_response;
        return false;
    }

    sclient.print(String("GET ") + path + " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection: close\r\n\r\n");

    while (sclient.connected() && !sclient.available()) delay(10);
    
    // Skip headers
    if (!sclient.find("\r\n\r\n")) {
         sysStatus.last_server_response = "OTA-WIFI Error: Invalid HTTP response.\n" + sysStatus.last_server_response;
         sclient.stop();
         return false;
    }

    String payload = sclient.readString();
    sclient.stop();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        sysStatus.last_server_response = "OTA-WIFI Error: JSON parse failed.\n" + sysStatus.last_server_response;
        return false;
    }

#ifdef LILYGO_T_A7670
    const char* myHw = "T-A7670X";
#else
    const char* myHw = "Generic";
#endif

    if (!doc[myHw].is<JsonObject>()) {
        sysStatus.last_server_response = "OTA-WIFI Info: No entry for " + String(myHw) + ".\n" + sysStatus.last_server_response;
        return false;
    }

    JsonObject entry = doc[myHw];
    const char* newVersion = entry["version"];
    const char* binaryUrl = entry["url"];
    size_t binSize = entry["size"] | 0;

    if (String(newVersion) == String(APP_VERSION)) {
        sysStatus.last_server_response = "OTA-WIFI: System up to date (v" + String(APP_VERSION) + ").\n" + sysStatus.last_server_response;
        return false;
    }

    // 2. Download BIN
    // Extract host and path from binaryUrl
    String binUrl = String(binaryUrl);
    int hostStart = binUrl.indexOf("://") + 3;
    int pathStart = binUrl.indexOf("/", hostStart);
    String binHost = binUrl.substring(hostStart, pathStart);
    String binPath = binUrl.substring(pathStart);

    sysStatus.last_server_response = "OTA-WIFI: Downloading v" + String(newVersion) + "...\n" + sysStatus.last_server_response;
    
    if (!sclient.connect(binHost.c_str(), 443)) {
        sysStatus.last_server_response = "OTA-WIFI Error: BIN host connection failed.\n" + sysStatus.last_server_response;
        return false;
    }

    sclient.print(String("GET ") + binPath + " HTTP/1.1\r\n" +
                 "Host: " + binHost + "\r\n" +
                 "Connection: close\r\n\r\n");

    while (sclient.connected() && !sclient.available()) delay(10);
    if (!sclient.find("\r\n\r\n")) {
         sysStatus.last_server_response = "OTA-WIFI Error: BIN HTTP error.\n" + sysStatus.last_server_response;
         sclient.stop();
         return false;
    }

    if (!Update.begin(binSize)) {
        sysStatus.last_server_response = "OTA-WIFI Error: Flash space too low.\n" + sysStatus.last_server_response;
        sclient.stop();
        return false;
    }

    size_t total = 0;
    uint8_t buffer[1024];
    while (sclient.connected() || sclient.available()) {
        while (sclient.available()) {
            int c = sclient.read(buffer, sizeof(buffer));
            Update.write(buffer, c);
            total += c;
            int progress = (total * 100) / binSize;
            static int lastP = -1;
            if (progress / 10 != lastP) {
                sysStatus.last_server_response = "OTA-WIFI: " + String(progress) + "&#37;\n" + sysStatus.last_server_response;
                lastP = progress / 10;
            }
        }
    }

    sclient.stop();
    if (Update.end(true)) {
        sysStatus.last_server_response = "OTA-WIFI SUCCESS! Rebooting...\n" + sysStatus.last_server_response;
        delay(2000);
        ESP.restart();
    } else {
        sysStatus.last_server_response = "OTA-WIFI FAIL: #" + String(Update.getError()) + "\n" + sysStatus.last_server_response;
    }

    return true;
}

void checkAndPerformAutoOTA() {
#if ENABLE_AUTO_OTA
    Serial.println(">>> CHECKING FOR AUTOMATED OTA UPDATE <<<");
    
    // 1. Check if we have a valid timestamp from the server
    if (sysStatus.last_server_timestamp == 0) {
        Serial.println("OTA: No server timestamp available. Skipping check.");
        return;
    }

    // 2. Check if a full interval has passed since the last check
    uint32_t intervalSec = (uint32_t)AUTO_OTA_CHECK_INTERVAL_H * 3600;
    if (lastOTACheckTimestamp > 0) {
        // Simple 24h logic based on Unix timestamps from server response
        if (sysStatus.last_server_timestamp < lastOTACheckTimestamp) {
            // Server timestamp reset or invalid?
            lastOTACheckTimestamp = sysStatus.last_server_timestamp;
            return;
        }
        uint32_t elapsed = sysStatus.last_server_timestamp - lastOTACheckTimestamp;
        if (elapsed < intervalSec) {
            uint32_t hoursLeft = (intervalSec - elapsed) / 3600;
            Serial.printf("OTA: Next check in ~%d hours.\n", hoursLeft);
            return;
        }
    }

    // 3. Condition: Battery must be good
    if (currentBatteryVoltage < AUTO_OTA_MIN_BATTERY) {
        Serial.printf("OTA: Battery too low (%.2fV < %.2fV). Skipping update.\n", currentBatteryVoltage, AUTO_OTA_MIN_BATTERY);
        return;
    }

    // 4. Condition: Daylight/Solar OR Time-of-Day
    bool isDaytime = true; // Assume true if no solar check and no time check
    
    // Check Solar if required
    #if AUTO_OTA_REQUIRE_SOLAR
    if (currentSolarVoltage < AUTO_OTA_MIN_SOLAR) {
        Serial.printf("OTA: Solar too low (%.2fV < %.2fV). Skipping update.\n", currentSolarVoltage, AUTO_OTA_MIN_SOLAR);
        return;
    }
    #endif

    // Check Time of Day (Target around noon e.g. 12:00 +/- window)
    time_t t = (time_t)sysStatus.last_server_timestamp;
    struct tm* timeinfo = gmtime(&t); // Using UTC from server
    int currentHour = timeinfo->tm_hour; // Note: this is UTC. We might need offset, but 12:00 UTC is usually middle of day in Europe too.
    
    int minHour = AUTO_OTA_PREFERRED_HOUR - AUTO_OTA_HOUR_WINDOW;
    int maxHour = AUTO_OTA_PREFERRED_HOUR + AUTO_OTA_HOUR_WINDOW;
    
    if (currentHour < minHour || currentHour > maxHour) {
        Serial.printf("OTA: Not in preferred time window (Hour: %02d:00 UTC, Preferred: %02d:00 +/- %d). Skipping.\n", currentHour, AUTO_OTA_PREFERRED_HOUR, AUTO_OTA_HOUR_WINDOW);
        return;
    }

    // 5. Conditions met! Update last check timestamp and perform update
    Serial.println("OTA: All conditions met. Performing update check...");
    lastOTACheckTimestamp = sysStatus.last_server_timestamp; // Mark as checked even if no update is found
    
    // Perform update
    performModemOTA(OTA_MANIFEST_URL);
#else
    Serial.println("OTA: Automated check is disabled in config.h");
#endif
}
