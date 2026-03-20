#ifndef BEELOGGER_CONFIG_H
#define BEELOGGER_CONFIG_H

#include <Arduino.h>

// ==========================================
// APPLICATION CONFIGURATION
// ==========================================
#define APP_VERSION                         "1.0.0"

// Board pins and modem model selection is handled via platformio.ini 
// flags and utilities.h
#include "utilities.h"

// Mapping Beelogger-specific names from utilities.h:
#ifndef MODEM_PWRKEY_PIN
#define MODEM_PWRKEY_PIN      BOARD_PWRKEY_PIN
#endif
#ifndef MODEM_POWERON_PIN
#define MODEM_POWERON_PIN     BOARD_POWERON_PIN
#endif
#ifndef SYSTEM_POWER_PIN
#define SYSTEM_POWER_PIN      BOARD_POWERON_PIN
#endif

// Power supply measurement configuration
#ifndef BOARD_BAT_ADC_PIN
#define BOARD_BAT_ADC_PIN     35 
#endif
#define BAT_VOLTAGE_DIVIDER                 2.0f // Multiplier for actual voltage

#ifndef BOARD_SOLAR_ADC_PIN
#define BOARD_SOLAR_ADC_PIN   36 
#endif
#define SOLAR_VOLTAGE_DIVIDER               2.0f 

// ==========================================
// OVERRIDE FOR LOCAL CREDENTIALS
// ==========================================
// If config.local.h exists (ignored by git), it will override settings below
#if __has_include("config.local.h")
    #include "config.local.h"
#endif

// ==========================================
// USER CUSTOMIZABLE SENSOR PINS
// ==========================================
// I2C Pins
#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN                         BOARD_SDA_PIN
#endif
#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN                         BOARD_SCL_PIN
#endif

// BME280 Settings
#define BME280_I2C_ADDRESS                  0x76 // Common: 0x76 or 0x77
#define ENABLE_BME280                       true

// HX711 Loadcell Settings
#define HX711_SCK_PIN                       19
#define HX711_DOUT_PIN                      23
#define HX711_POWER_PIN                     18 // Pin to control power (optional)
#define ENABLE_HX711                        true

// HX711 DEFAULT FALLBACK VALUES (stored in NVS after calibration)
#define DEFAULT_LOADCELL_OFFSET             50682624L
#define DEFAULT_LOADCELL_DIVIDER            5895655.0f
#define ENABLE_CALIBRATION_MODE             false // Set to true for serial calibration on boot

// ==========================================
// MEASUREMENT & SLEEP CONFIGURATION
// ==========================================
#define DEEP_SLEEP_MINUTES                  5    // Time spent in deep sleep
#define ACTIVE_AWAKE_TIMEOUT_SEC            30   // Time awake (web UI active) before sleeping
#define uS_TO_S_FACTOR 1000000ULL                 // Factor for ESP32 deep sleep timer

// ==========================================
// CELLULAR / DATA BACKEND CREDENTIALS
// ==========================================
#ifndef APN
#define APN                                 "your-apn.com"
#endif
#ifndef GPRS_USER
#define GPRS_USER                           ""
#endif
#ifndef GPRS_PASS
#define GPRS_PASS                           ""
#endif

#ifndef SERVER_HOST
#define SERVER_HOST                         "your-beelogger-server.com"
#endif
#ifndef SERVER_PATH
#define SERVER_PATH                         "/data"
#endif
#ifndef SERVER_PORT
#define SERVER_PORT                         8081
#endif

// ==========================================
// WIFI & DEBUG WEB INTERFACE
// ==========================================
#define ENABLE_DEBUG_WEB                    true  // Set to true to enable Local WiFi Debug Page
#define DEBUG_WIFI_USE_AP                   false // true = Own Access Point (AP), false = STA (Router)

#ifndef DEBUG_WIFI_SSID
#define DEBUG_WIFI_SSID                     "Your_WiFi_SSID"
#endif
#ifndef DEBUG_WIFI_PASS
#define DEBUG_WIFI_PASS                     "Your_WiFi_Password"
#endif

#endif // BEELOGGER_CONFIG_H
