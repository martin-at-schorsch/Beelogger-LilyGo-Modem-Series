#include "modem.h"
#include "config.h"

// Modem selection and SerialAT definition is now handled by utilities.h
#include <TinyGsmClient.h>

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
    if (!modem.waitForNetwork(60000L)) { // 1 minute timeout
        Serial.println(" timeout/fail");
        sysStatus.network_connected = false;
        return false;
    }
    Serial.println(" successful");
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
        sysStatus.network_connected = false;
        return false;
    }
    Serial.println("connected");
    sysStatus.network_connected = true;
    return true;
}

bool postData(String payload) {
    // Sanitize values
    payload.replace("nan", "0.00");
    
    sysStatus.last_payload = payload;
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

    unsigned long timeout = millis();
    String response = "";
    while (client.connected() && millis() - timeout < 10000L) {
        while (client.available()) {
            char c = client.read();
            response += c;
            Serial.print(c);
            timeout = millis();
        }
    }
    client.stop();
    sysStatus.last_server_response = response;
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
    sysStatus.modem_info = "Offline";
}
