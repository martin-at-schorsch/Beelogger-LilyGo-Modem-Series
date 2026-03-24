#ifndef BEELOGGER_WEB_SERVER_H
#define BEELOGGER_WEB_SERVER_H

#include <Arduino.h>

void startWorkModeWebServer();
void handleWebCommands();
bool isWorkModeEnabled();

extern bool workModeActive;

enum WebCommand {
    CMD_NONE = 0,
    CMD_TARE,
    CMD_TARE_RESET,
    CMD_CALIBRATE,
    CMD_REFRESH,
    CMD_MODEM_ON,
    CMD_MODEM_OFF,
    CMD_MODEM_RESTART,
    CMD_MODEM_SEND,
    CMD_STAY_AWAKE_ON,
    CMD_STAY_AWAKE_OFF,
    CMD_REBOOT,
    CMD_OTA_MODEM,
    CMD_OTA_WIFI
};

extern WebCommand pendingCommand;
extern float pendingCalWeight;
extern bool stayAwake;
extern unsigned long awakeTimerStart;
extern uint32_t activeTimeout;

#endif // BEELOGGER_WEB_SERVER_H
