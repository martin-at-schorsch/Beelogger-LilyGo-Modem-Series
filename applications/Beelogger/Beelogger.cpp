#include <Arduino.h>
#include "config.h"
#include "sensors.h"
#include "modem.h"
#include "web_server.h"

// Define Wakeup Source for ESP32
RTC_DATA_ATTR int bootCount = 0;

void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:     Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1:     Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER:    Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP:      Serial.println("Wakeup caused by ULP program"); break;
    default:                        Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}

#include <nvs_flash.h>

void setup() {
    Serial.begin(115200);
    delay(100); 

    // Print App Version
    Serial.println("\n----------------------------------");
    Serial.print("BEELOGGER PRO | Firmware: ");
    Serial.println(APP_VERSION);
    Serial.println("----------------------------------");

    // Initialize System Power Pin for peripherals
#if defined(SYSTEM_POWER_PIN) && SYSTEM_POWER_PIN >= 0
    pinMode(SYSTEM_POWER_PIN, OUTPUT);
    digitalWrite(SYSTEM_POWER_PIN, HIGH);
#endif

    delay(900); 

    // Initialisiere NVS Flash (required for calibration storage)
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    
    bootCount++;
    Serial.println("Boot number: " + String(bootCount));
    print_wakeup_reason();

#if ENABLE_CALIBRATION_MODE
    Serial.println("\n*** CALIBRATION MODE ENABLED ***");
    Serial.println("Skipping normal cycle. Launching HX711 Setup...");
    calibrateHX711();
    while(true) { delay(1000); } // Halt execution
#endif

    // 1. Initialize and Read Sensors
    Serial.println("Setting up sensors...");
    setupSensors();
    delay(500); // Stabilization

#if ENABLE_DEBUG_WEB
    startDebugWebServer();
#endif

    Serial.println("Reading data from sensors...");
    readSensors();

    // Construct Payload using standardized Beelogger keys
    // T1: Temperature, F1: Humidity, A1: Pressure, G1: Weight, VB: Battery, VS: Solar
    String payload = "T1=" + String(currentTemperature, 1) + 
                     "&F1=" + String(currentHumidity, 1) +
                     "&A1=" + String(currentPressure, 1) +
                     "&G1=" + String(currentWeight, 2) +
                     "&VB=" + String(currentBatteryVoltage, 2) +
                     "&VS=" + String(currentSolarVoltage, 2);

    float checkSumVal = currentTemperature + currentHumidity + currentPressure + currentWeight + currentBatteryVoltage + currentSolarVoltage;
    int checkInt = (int)round(checkSumVal + 0.5);
    payload += "&C=" + String(checkInt);
    payload += "&PW=" + String("beelogger"); 

    Serial.println("--- Payload Preview ---");
    Serial.printf(" Temp: %.1f C, Hum: %.1f %%, Pres: %.1f hPa\n", currentTemperature, currentHumidity, currentPressure);
    Serial.printf(" Weight: %.2f kg, Battery: %.2f V, Solar: %.2f V\n", currentWeight, currentBatteryVoltage, currentSolarVoltage);
    Serial.println("Raw Payload: " + payload);
    Serial.println("------------------------");

    // 2. Initialize Modem and Send Data
    Serial.println("Switching on Modem...");
    setupModem();

    if (connectNetwork()) {
        postData(payload);
    } else {
        Serial.println("Network failure. Skipping data transmission.");
    }

    // 3. Power Down Modem
    powerDownModem();

#if ENABLE_DEBUG_WEB
    Serial.printf("Debug Web UI active. Waiting %d seconds before sleep...\n", ACTIVE_AWAKE_TIMEOUT_SEC);
    awakeTimerStart = millis();
    while (millis() - awakeTimerStart < (ACTIVE_AWAKE_TIMEOUT_SEC * 1000ULL)) {
        // Handle commands from the asynchronous web server
        handleWebCommands();
        
        // If "Stay Awake" is toggled on via UI, reset timer
        if (stayAwake) {
            awakeTimerStart = millis(); 
        }
        
        delay(100);
    }
#endif

    // 4. Enter Deep Sleep
    uint64_t timeToSleep = (uint64_t)DEEP_SLEEP_MINUTES * 60 * uS_TO_S_FACTOR;
    Serial.printf("System entering deep sleep for %d minutes...\n", DEEP_SLEEP_MINUTES);
    esp_sleep_enable_timer_wakeup(timeToSleep);
    Serial.flush(); 
    powerDownSensors();
    
    // Shut off general system power pin in sleep
#if defined(SYSTEM_POWER_PIN) && SYSTEM_POWER_PIN >= 0
    digitalWrite(SYSTEM_POWER_PIN, LOW);
#endif

    esp_deep_sleep_start();
}

void loop() {
    // Empty, loop not reached during deep sleep cycles
}
