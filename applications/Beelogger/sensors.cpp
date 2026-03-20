#include "sensors.h"
#include "config.h"
#include <Wire.h>

#if ENABLE_BME280
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
Adafruit_BME280 bme; 
#endif

#if ENABLE_HX711
#include "HX711.h"
#include <Preferences.h>
HX711 scale;
long currentLoadcellOffset = DEFAULT_LOADCELL_OFFSET;
float currentLoadcellDivider = DEFAULT_LOADCELL_DIVIDER;
#endif

float currentTemperature = -99.0;
float currentHumidity = -99.0;
float currentPressure = -99.0;
float currentWeight = -99.0;
long currentWeightRaw = 0;
float currentBatteryVoltage = -99.0;
float currentSolarVoltage = -99.0;

void setupSensors() {
    // I2C Initialize
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

#if ENABLE_BME280
    if (!bme.begin(BME280_I2C_ADDRESS, &Wire)) {
        Serial.println("Missing BME280 sensor - please check I2C wiring!");
    }
#endif

#if ENABLE_HX711
    Preferences preferences;
    // Load calibration from Non-Volatile Storage (NVS)
    preferences.begin("beelogger", false);
    
    if (!preferences.isKey("hx_offset")) preferences.putLong("hx_offset", DEFAULT_LOADCELL_OFFSET);
    if (!preferences.isKey("hx_divider")) preferences.putFloat("hx_divider", DEFAULT_LOADCELL_DIVIDER);

    currentLoadcellOffset = preferences.getLong("hx_offset", DEFAULT_LOADCELL_OFFSET);
    currentLoadcellDivider = preferences.getFloat("hx_divider", DEFAULT_LOADCELL_DIVIDER);
    
    // Safety check: Prevent division by zero
    if (abs(currentLoadcellDivider) < 0.0001f) {
        Serial.print("Warning: NVS Calibration data is invalid! Resetting to default divider: ");
        currentLoadcellDivider = DEFAULT_LOADCELL_DIVIDER;
        Serial.println(currentLoadcellDivider);
    }
    
    Serial.print("Calibration Loaded: Offset=");
    Serial.print(currentLoadcellOffset);
    Serial.print(", Divider=");
    Serial.println(currentLoadcellDivider);

    preferences.end();

#if defined(HX711_POWER_PIN) && HX711_POWER_PIN >= 0
    pinMode(HX711_POWER_PIN, OUTPUT);
    digitalWrite(HX711_POWER_PIN, HIGH);
    delay(200);
#endif

    scale.begin(HX711_DOUT_PIN, HX711_SCK_PIN);
    scale.set_scale(currentLoadcellDivider);
    scale.set_offset(currentLoadcellOffset);
#endif

    // ADC Resolution
    analogReadResolution(12); // ESP32 12-bit ADC
}

void readBatteryAndSolar() {
    // Voltage dividers are calibrated for 12-bit ADC (0-4095)
    int batAdc = analogRead(BOARD_BAT_ADC_PIN);
    currentBatteryVoltage = (batAdc / 4095.0) * 3.3 * BAT_VOLTAGE_DIVIDER;

#if defined(BOARD_SOLAR_ADC_PIN) && BOARD_SOLAR_ADC_PIN >= 0
    int solAdc = analogRead(BOARD_SOLAR_ADC_PIN);
    currentSolarVoltage = (solAdc / 4095.0) * 3.3 * SOLAR_VOLTAGE_DIVIDER;
#endif
}

void readSensors() {
    Serial.println("Polling sensor data...");
    readBatteryAndSolar();

#if ENABLE_BME280
    currentTemperature = bme.readTemperature();
    currentHumidity = bme.readHumidity();
    currentPressure = bme.readPressure() / 100.0F; // hPa
#endif

#if ENABLE_HX711
    if (scale.wait_ready_timeout(2000)) {
        currentWeight = scale.get_units(10); // Average of 10 samples
        currentWeightRaw = scale.read_average(5);
    } else {
        Serial.println("HX711 load cell is not responding!");
        currentWeight = -1.0;
    }
#endif
}

#if ENABLE_HX711
void calibrateHX711() {
    Serial.println("\n--- HX711 Calibration Assistant ---");
    Serial.println("1. Clear the scale (Remove all weight/vandalism).");
    Serial.println("2. Send 'c' or 'C' in Serial Monitor to continue...");

    while (!Serial.available() || (Serial.read() != 'c' && Serial.read() != 'C')) {
        delay(10);
    }
    while (Serial.available()) Serial.read(); // Clear input

#if defined(HX711_POWER_PIN) && HX711_POWER_PIN >= 0
    digitalWrite(HX711_POWER_PIN, HIGH);
    delay(200);
#endif

    scale.begin(HX711_DOUT_PIN, HX711_SCK_PIN);
    scale.set_scale();
    scale.tare(); // Zero out

    long newOffset = scale.get_offset();
    Serial.print("Calculated Zero Factor (Offset): ");
    Serial.println(newOffset);

    Serial.println("\n1. Place a KNOWN weight (e.g., 1000g) on the scale.");
    Serial.println("2. Enter the weight in grams (e.g., 500) and press Enter...");

    String input = "";
    while (true) {
        if (Serial.available()) {
            char c = Serial.read();
            if (c == '\n' || c == '\r') {
                if (input.length() > 0) break;
            } else {
                input += c;
            }
        }
        delay(10);
    }
    float knownWeight = input.toFloat();
    Serial.print("Calibration Weight: ");
    Serial.println(knownWeight);

    // Get raw reading
    float rawValue = scale.get_units(10);
    float newDivider = rawValue / knownWeight;

    Serial.print("New Scaling Factor (Divider): ");
    Serial.println(newDivider);

    // Persist to NVS
    Preferences preferences;
    preferences.begin("beelogger", false);
    preferences.putLong("hx_offset", newOffset);
    preferences.putFloat("hx_divider", newDivider);
    preferences.end();

    Serial.println("\nCalibration complete! Saved to storage.");
    Serial.println("Ensure ENABLE_CALIBRATION_MODE is false in config.h for normal operation.");
    
    // Testing loop
    scale.set_scale(newDivider);
    scale.set_offset(newOffset);
    Serial.println("\nLive testing weight. Reset board to finish...");
    while(true) {
        Serial.print("Weight: ");
        Serial.println(scale.get_units(10));
        delay(1000);
    }
}

void tareScale() {
    Serial.println("WEB-CMD: Tare/Zeroing scale...");
#if ENABLE_HX711
    scale.tare();
    currentLoadcellOffset = scale.get_offset();
    
    Preferences preferences;
    preferences.begin("beelogger", false);
    preferences.putLong("hx_offset", currentLoadcellOffset);
    preferences.end();
    
    readSensors();
#endif
}

void calibrateScale(float knownWeight) {
    Serial.print("WEB-CMD: Calibration request with ");
    Serial.print(knownWeight);
    Serial.println("g...");
#if ENABLE_HX711
    if (knownWeight <= 0) return;

    scale.set_scale(1.0);
    float rawValue = scale.get_units(10);
    
    if (abs(rawValue) < 0.001) {
        Serial.println("ERROR: Scale reading is zero. Is the sensor unplugged?");
        return;
    }

    float newDivider = rawValue / knownWeight;
    if (isnan(newDivider) || isinf(newDivider)) {
        Serial.println("ERROR: Invalid scaling factor!");
        return;
    }

    scale.set_scale(newDivider);
    currentLoadcellDivider = newDivider;
    
    Preferences preferences;
    preferences.begin("beelogger", false);
    preferences.putFloat("hx_divider", currentLoadcellDivider);
    preferences.end();
    
    readSensors();
#endif
}

void powerDownSensors() {
    Serial.println("Shutting down sensor power pins...");
#if ENABLE_HX711 && defined(HX711_POWER_PIN) && HX711_POWER_PIN >= 0
    digitalWrite(HX711_POWER_PIN, LOW);
#endif
}

void resetScaleCalibration() {
    Serial.println("WEB-CMD: Restoring scale defaults...");
#if ENABLE_HX711
    currentLoadcellOffset = DEFAULT_LOADCELL_OFFSET;
    currentLoadcellDivider = DEFAULT_LOADCELL_DIVIDER;
    
    scale.set_offset(currentLoadcellOffset);
    scale.set_scale(currentLoadcellDivider);
    
    Preferences preferences;
    preferences.begin("beelogger", false);
    preferences.putLong("hx_offset", currentLoadcellOffset);
    preferences.putFloat("hx_divider", currentLoadcellDivider);
    preferences.end();
    
    readSensors();
    Serial.println("Scale reset successful.");
#endif
}
#else
void calibrateHX711() {}
void tareScale() {}
void calibrateScale(float knownWeight) {}
#endif
