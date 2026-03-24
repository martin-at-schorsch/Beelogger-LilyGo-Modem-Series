#ifndef BEELOGGER_MODEM_H
#define BEELOGGER_MODEM_H

#include <Arduino.h>

struct SystemStatus {
    bool modem_powered_on = false;
    bool modem_init_success = false;
    bool network_connected = false;
    bool server_connected = false;
    int last_http_code = -1;
    float cell_rssi = 0;
    String network_type = "Unknown";
    String last_server_response = "";
    String last_payload = "";
    unsigned long last_payload_time = 0;
    uint32_t last_server_timestamp = 0;
    String modem_info = "Unknown";
};

extern bool workModeActive;
extern uint32_t activeTimeout;
extern SystemStatus sysStatus;
extern RTC_DATA_ATTR uint32_t lastOTACheckTimestamp;

void setupModem();
bool connectNetwork();
bool postData(String payload);
void powerDownModem();
bool performModemOTA(const char* manifestUrl);
bool performWifiOTA(const char* manifestUrl);
void checkAndPerformAutoOTA();

#endif // BEELOGGER_MODEM_H
