#ifndef BEELOGGER_WEB_SERVER_H
#define BEELOGGER_WEB_SERVER_H

#include <Arduino.h>

void startDebugWebServer();
void startDebugWebServer();
void handleDebugWebServer();
void handleWebCommands(); // Neue Funktion für den Main-Loop
bool isWebDebugEnabled();

enum WebCommand {
    CMD_NONE = 0,
    CMD_TARE,
    CMD_CALIBRATE,
    CMD_REFRESH,
    CMD_MODEM_ON,
    CMD_MODEM_OFF,
    CMD_MODEM_RESTART,
    CMD_MODEM_SEND,
    CMD_STAY_AWAKE_ON,
    CMD_STAY_AWAKE_OFF,
    CMD_RESET_CAL,
    CMD_REBOOT
};

extern WebCommand pendingCommand;
extern float pendingCalWeight;
extern bool stayAwake;
extern unsigned long awakeTimerStart;

#endif // BEELOGGER_WEB_SERVER_H
