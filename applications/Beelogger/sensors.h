#ifndef BEELOGGER_SENSORS_H
#define BEELOGGER_SENSORS_H

#include <Arduino.h>

void setupSensors();
void readSensors();
void powerDownSensors();
void calibrateHX711();
void tareScale();
void calibrateScale(float knownWeight);
void resetScaleCalibration();

extern float currentTemperature;
extern float currentHumidity;
extern float currentPressure;
extern float currentWeight;
extern long currentWeightRaw;
extern long currentLoadcellOffset;
extern float currentLoadcellDivider;

extern float currentBatteryVoltage;
extern float currentSolarVoltage;

#endif // BEELOGGER_SENSORS_H
