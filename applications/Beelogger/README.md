# Beelogger ESP32 - LilyGo Modem Series

Professional Beehive Monitoring System based on ESP32 and LilyGo T-SIM/T-Call Modem series.

## Features
- **Multi-Board Support**: Automatically detects LilyGo modem boards via `utilities.h`.
- **Sensors**: Supports HX711 Load Cells (Weight) and BME280 (Temp/Hum/Pres).
- **Cellular Data**: Transmits data to a Beelogger-compatible backend via LTE/GSM.
- **Robust Test Server**: Multi-threaded Python server included for local testing and debugging with detailed access logging.
- **Work-on-Hive Mode**: Special maintenance mode triggered by the **BOOT button (GPIO0)**.
  - Activates Local Web Interface for 5 minutes.
  - Disables Deep Sleep during maintenance.
- **OTA System**:
  OTA via LTE/GSM automatic, manual or via Work-on-Hive Mode Website. 
  - **Automated Background OTA**: Checks every 24h for updates during safe windows (daylight/battery > 3.8V).
  - **Manual OTA**: Cloud or local firmware upload via the Web Interface.
- **Power Management**: Advanced deep sleep logic with battery and solar monitoring.

## Configuration
Modify `config.h` and create a `config.local.h` for your credentials.

### Automated background Updates
The system checks every 24 hours for a new version. To ensure reliability:
- **Battery Check**: Updates only start if battery > 3.8V.
- **Daylight Check**: Uses solar voltage or a preferred time window (e.g. 10:00 - 14:00) to ensure daylight.

## Changelog
### v1.1.0
- **Firmware**: Added automated 24h background OTA checks with battery/solar safety guards.
- **Firmware**: Implemented time-window detection via server-provided Unix timestamps.
- **Server**: Robustified `beelogger_test_server.py` with multi-threading to prevent freezes from bot scans.
- **Server**: Added `beelogger_access.log` to track IPs and request outcomes.
- **UI**: Added "Maintenance Active" banner and improved layout.
- **Feature**: Added "Stay Awake" toggle in Web UI.
- **Fix**: Improved modem power-cycle logic on boot.

### v1.0.0
- **Initial Release**: Basic Beelogger functionality for ESP32 and LilyGo modems.
- **Sensors**: Support for HX711 (Weight) and BME280 (Temp/Hum/Pres).
- **Backend**: Cellular data transmission to Beelogger.php compatible servers.
- **Feature**: Initial "Work-on-Hive" mode with Web Interface.

## Credits
Based on the LilyGo Modem Series examples and enhanced for professional beehive monitoring.
