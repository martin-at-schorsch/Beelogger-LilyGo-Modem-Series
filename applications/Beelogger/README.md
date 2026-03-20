# Beelogger Pro for LilyGo Modem Series

This application transforms a LilyGo T-SIM A7670 / T-Call / T-SIM S3 module into a professional **Beelogger** (Beehive Monitoring System). It measures weight, temperature, humidity, atmospheric pressure, and battery/solar voltages, transmitting them via LTE/GPRS to a web backend.

This is a implementation of the original [Beelogger project](https://beelogger.de) for the LilyGo Modem Series.

## 🚀 Features
- **Multi-Board Support**: Automatically configures pins for A7670, SIM7670G, SIM7000G, and many more LilyGo boards.
- **Sensor Integration**:
  - **HX711**: Precision weight measurement for the beehive.
  - **BME280**: Temperature, Humidity, and Pressure.
  - **Voltage Monitoring**: Tracks both Battery and Solar panel levels if supported by the board.
- **Efficient Power Management**: Uses ESP32 Deep Sleep and modem power-down cycles for long-term battery operation.
- **Web Dashboard**: Local WiFi-based debug interface for real-time monitoring and scale calibration.
- **Fail-Safe Connection**: Configurable LTE/GSM preferences and robust connection logic.

## 🛠️ Hardware Requirements
- **Board**: Tested on LilyGo T-SIM A7670E.
- **Sensors**: 
  - HX711 Load Cell Amplifier
  - BME280 I2C Sensor
- **Power**: LiPo Battery (3.7V) and optional Solar Panel.

## ⚙️ Configuration
The configuration is split into two parts for security:
1. **`config.h`**: Contains general application logic and default placeholders.
2. **`config.local.h`**: (Ignored by git) Put your private WiFi and Backend credentials here. 
   - *Tip: Rename a copy of the provided template or let the code create one.*

## 📐 Calibration
You can calibrate the scale via:
1. **Serial Console**: Set `ENABLE_CALIBRATION_MODE` to `true` in `config.h`.
2. **Web UI**: Access the local IP of the ESP32 while it's awake and use the "Scale Calibration" section.

## 📦 Deployment (PlatformIO)
Select your board environment in `platformio.ini` (e.g., `default_envs = T-A7670X`) and upload the firmware.
