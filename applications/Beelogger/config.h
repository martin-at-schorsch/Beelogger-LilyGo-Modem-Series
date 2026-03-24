#ifndef BEELOGGER_CONFIG_H
#define BEELOGGER_CONFIG_H

#include <Arduino.h>

// ==========================================
// APPLICATION CONFIGURATION
// ==========================================
#ifndef APP_VERSION
#define APP_VERSION                         "1.1.0-R22"
#endif

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
#ifndef BME280_I2C_ADDRESS
#define BME280_I2C_ADDRESS                  0x76 // Common: 0x76 or 0x77
#endif
#ifndef ENABLE_BME280
#define ENABLE_BME280                       1
#endif

// HX711 Loadcell Settings
#ifndef HX711_SCK_PIN
#define HX711_SCK_PIN                       19
#endif
#ifndef HX711_DOUT_PIN
#define HX711_DOUT_PIN                      23
#endif
#ifndef HX711_POWER_PIN
#define HX711_POWER_PIN                     18 // Pin to control power (optional)
#endif
#ifndef ENABLE_HX711
#define ENABLE_HX711                        1
#endif

// HX711 DEFAULT FALLBACK VALUES (stored in NVS after calibration)
#define DEFAULT_LOADCELL_OFFSET             50682624L
#define DEFAULT_LOADCELL_DIVIDER            5895655.0f
#define ENABLE_CALIBRATION_MODE             false // Set to true for serial calibration on boot

// ==========================================
// MEASUREMENT & SLEEP CONFIGURATION
// ==========================================
#ifndef DEEP_SLEEP_MINUTES
#define DEEP_SLEEP_MINUTES                  5    // Time spent in deep sleep
#endif
#ifndef WORK_MODE_TIMEOUT_MIN
#define WORK_MODE_TIMEOUT_MIN               5    // Time to stay awake in Work-on-Hive mode
#endif
#define uS_TO_S_FACTOR 1000000ULL                 // Factor for ESP32 deep sleep timer

// = ::::::::::::::::::::::::::::::::::::::
// BUTTON & INTERRUPT CONFIGURATION
// ::::::::::::::::::::::::::::::::::::::
#ifndef GPIO0_PIN
#define GPIO0_PIN                           0    // BOOT Button on most LilyGo boards
#endif

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

#ifndef OTA_MANIFEST_URL
#define OTA_MANIFEST_URL                    "https://your-server.com/beelogger/version.json"
#endif

#ifndef SECRET_OTA_TOKEN
#define SECRET_OTA_TOKEN                    "YourSecretToken"
#endif

// ==========================================
// AUTOMATIC FIRMWARE UPDATE (OTA)
// ==========================================
#ifndef ENABLE_AUTO_OTA
#define ENABLE_AUTO_OTA                     1   // Set to 1 to check for updates automatically
#endif
#ifndef AUTO_OTA_CHECK_INTERVAL_H
#define AUTO_OTA_CHECK_INTERVAL_H           24  // Check every 24 hours
#endif
#ifndef AUTO_OTA_MIN_BATTERY
#define AUTO_OTA_MIN_BATTERY                3.8f // Minimum battery voltage to start update
#endif
#ifndef AUTO_OTA_MIN_SOLAR
#define AUTO_OTA_MIN_SOLAR                  0.5f // Minimum solar voltage to detect daylight/charging
#endif
#ifndef AUTO_OTA_REQUIRE_SOLAR
#define AUTO_OTA_REQUIRE_SOLAR              0    // Set to 1 if solar voltage must be present for update
#endif
#ifndef AUTO_OTA_PREFERRED_HOUR
#define AUTO_OTA_PREFERRED_HOUR             12   // Target time (e.g. 12:00 noon)
#endif
#ifndef AUTO_OTA_HOUR_WINDOW
#define AUTO_OTA_HOUR_WINDOW                2    // +/- hours around preferred hour (e.g. 10-14h)
#endif

// ==========================================
// WIFI & WORK-ON-HIVE INTERFACE
// ==========================================
#ifndef ENABLE_WORK_ON_HIVE_WEB
#define ENABLE_WORK_ON_HIVE_WEB             1  // Set to 1 to enable Local Web Page
#endif
#ifndef WIFI_NORMAL_AWAKE_SEC
#define WIFI_NORMAL_AWAKE_SEC               60  // Set to > 0 to enable WiFi on every measurement cycle (e.g. 30)
#endif
#ifndef WIFI_WORK_MODE_AP
#define WIFI_WORK_MODE_AP                   0 // 1 = Own Access Point (AP), 0 = STA (Router)
#endif

#ifndef WIFI_STA_SSID
#define WIFI_STA_SSID                       "Your_WiFi_SSID"
#endif
#ifndef WIFI_STA_PASS
#define WIFI_STA_PASS                       "Your_WiFi_Password"
#endif

// AP Settings (used for AP mode or if STA connection fails)
#ifndef WIFI_AP_SSID
#define WIFI_AP_SSID                        "Beelogger-ESP32"
#endif
#ifndef WIFI_AP_PASS
#define WIFI_AP_PASS                        "Your_Beelogger-WiFi_Password_for_AP_and_Fallback"
#endif

// Diagnostic Settings
#ifndef ENABLE_WIFI_SCANNER
#define ENABLE_WIFI_SCANNER                 1 // Set to 1 to scan nearby networks during startup
#endif

#endif // BEELOGGER_CONFIG_H
