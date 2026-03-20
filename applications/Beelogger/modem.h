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
    String modem_info = "Unknown";
};

extern SystemStatus sysStatus;

void setupModem();
bool connectNetwork();
bool postData(String payload);
void powerDownModem();

#endif // BEELOGGER_MODEM_H
