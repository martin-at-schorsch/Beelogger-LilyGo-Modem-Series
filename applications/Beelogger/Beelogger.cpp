#include <Arduino.h>
#include "config.h"
#include "sensors.h"
#include "modem.h"
#include "web_server.h"

// Define Wakeup Source for ESP32
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR uint32_t lastOTACheckTimestamp = 0;

volatile bool buttonInterruptFired = false;
void IRAM_ATTR handleButtonInterrupt() {
    buttonInterruptFired = true;
}

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
    Serial.print("BEELOGGER ESP32 | Firmware: ");
    Serial.println(APP_VERSION);
    Serial.println("----------------------------------");

    // Initialize System Power Pin for peripherals
#if defined(SYSTEM_POWER_PIN) && SYSTEM_POWER_PIN >= 0
    pinMode(SYSTEM_POWER_PIN, OUTPUT);
    digitalWrite(SYSTEM_POWER_PIN, HIGH);
#endif

    delay(900); 

    // Initialize NVS Flash (required for calibration storage)
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    
    bootCount++;
    Serial.println("Boot number: " + String(bootCount));
    
    // Check if woken up by Button (GPIO0)
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
        workModeActive = true;
        Serial.println("\n>>> WORK-ON-HIVE MODE ACTIVATED BY BUTTON <<<");
    }
    
    // Attach interrupt immediately so button presses during the 30s modem init are caught
    pinMode(GPIO0_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(GPIO0_PIN), handleButtonInterrupt, FALLING);
    
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

#if ENABLE_WORK_ON_HIVE_WEB
    bool shouldStartWifi = workModeActive || (WIFI_NORMAL_AWAKE_SEC > 0);
    if (shouldStartWifi) {
        activeTimeout = workModeActive ? (WORK_MODE_TIMEOUT_MIN * 60) : WIFI_NORMAL_AWAKE_SEC;
        awakeTimerStart = millis(); // Initialize early so the UI knows
        startWorkModeWebServer();
    }
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
    payload += "&C=" + String((int)round(checkSumVal + 0.5));
    payload += "&PW=beelogger";

    // Set payload to sysStatus so Web UI can display it even if network fails
    payload.replace("nan", "0.00");
    sysStatus.last_payload = payload;
    sysStatus.last_payload_time = millis();
    sysStatus.last_server_response = "";

    Serial.println("--- Payload Preview ---");
    Serial.printf(" Temp: %.1f C, Hum: %.1f %%, Pres: %.1f hPa\n", currentTemperature, currentHumidity, currentPressure);
    Serial.printf(" Weight: %.2f kg, Battery: %.2f V, Solar: %.2f V\n", currentWeight, currentBatteryVoltage, currentSolarVoltage);
    Serial.println("Raw Payload: " + payload);
    Serial.println("------------------------");

    // 2. Initialize Modem and Send Data
    // SKIP this if we are in Work-on-Hive mode to save time and battery
    if (!workModeActive) {
        Serial.println("Switching on Modem...");
        setupModem();

        if (connectNetwork()) {
            postData(payload);
        } else {
            Serial.println("Network failure. Skipping data transmission.");
        }

        // 3. Power Down Modem
        if (sysStatus.server_connected) {
            checkAndPerformAutoOTA();
        }
        powerDownModem();
    } else {
        Serial.println(">>> Work-Mode: Skipping initial cellular upload to launch WebServer immediately.");
    }

#if ENABLE_WORK_ON_HIVE_WEB
    // Check if button was pressed during the 30-sec blocking setup
    if (buttonInterruptFired) {
        Serial.println(">>> Button pressed during boot! Upgrading to Work-on-Hive Mode.");
        workModeActive = true;
        
        if (!shouldStartWifi) {
            shouldStartWifi = true;
            activeTimeout = WORK_MODE_TIMEOUT_MIN * 60;
            awakeTimerStart = millis();
            startWorkModeWebServer();
        } else {
            // Already started, so just upgrade timeout
            activeTimeout = WORK_MODE_TIMEOUT_MIN * 60;
            awakeTimerStart = millis();
        }
    }

    if (shouldStartWifi) {
        // Only override if not already upgraded by the interrupt
        if (!buttonInterruptFired) {
            activeTimeout = workModeActive ? (WORK_MODE_TIMEOUT_MIN * 60) : WIFI_NORMAL_AWAKE_SEC;
        }
        
        if (activeTimeout > 0) {
            Serial.printf("Work-on-Hive Interface active. Waiting %d seconds before sleep...\n", activeTimeout);
            // awakeTimerStart is already running since web UI startup, but we can reset or keep it
            while (millis() - awakeTimerStart < (activeTimeout * 1000ULL)) {
                // Handle commands from the asynchronous web server
                handleWebCommands();
                
                // If "Stay Awake" is toggled on via UI, reset timer
                if (stayAwake) {
                    awakeTimerStart = millis(); 
                }

                // Check if GPIO0 is pressed during active mode -> extend session to full 5 minutes
                if (digitalRead(GPIO0_PIN) == LOW) {
                    static unsigned long lastBtn = 0;
                    if (millis() - lastBtn > 500) {
                        Serial.println(">>> Button pressed: Extending maintenance session to 5 minutes...");
                        activeTimeout = WORK_MODE_TIMEOUT_MIN * 60;
                        workModeActive = true; // Mark as active if it wasn't already
                        awakeTimerStart = millis();
                        lastBtn = millis();
                    }
                }
                
                delay(100);
            }
        }
    }
#endif

    // 4. Enter Deep Sleep
    uint64_t timeToSleep = (uint64_t)DEEP_SLEEP_MINUTES * 60 * uS_TO_S_FACTOR;
    Serial.printf("System entering deep sleep for %d minutes...\n", DEEP_SLEEP_MINUTES);
    
    // Enable Wakeup by Button (GPIO0)
    esp_sleep_enable_ext0_wakeup((gpio_num_t)GPIO0_PIN, 0); // Wakeup when button is pressed (LOW)
    
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
