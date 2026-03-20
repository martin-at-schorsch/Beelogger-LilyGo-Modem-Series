/**
 * @file      utilities.h
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2025  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2025-04-30
 *
 */

#pragma once

#if defined(LILYGO_T_A7670)

    #define MODEM_BAUDRATE                      (115200)
    #define MODEM_DTR_PIN                       (25)
    #define MODEM_TX_PIN                        (26)
    #define MODEM_RX_PIN                        (27)
    // The modem boot pin needs to follow the startup sequence.
    #define BOARD_PWRKEY_PIN                    (4)
    // The modem power switch must be set to HIGH for the modem to supply power.
    #define BOARD_POWERON_PIN                   (12)
    #define MODEM_RING_PIN                      (33)
    #define MODEM_RESET_PIN                     (5)
    #define BOARD_MISO_PIN                      (2)
    #define BOARD_MOSI_PIN                      (15)
    #define BOARD_SCK_PIN                       (14)
    #define BOARD_SD_CS_PIN                     (13)
    #define BOARD_BAT_ADC_PIN                   (35)
    #define MODEM_RESET_LEVEL                   HIGH
    #define SerialAT                            Serial1

    #define MODEM_GPS_ENABLE_GPIO               (-1)
    #define MODEM_GPS_ENABLE_LEVEL              (-1)

    #ifndef TINY_GSM_MODEM_A7670
        #define TINY_GSM_MODEM_A7670
    #endif

    // It is only available in V1.4 version. In other versions, IO36 is not connected.
    #define BOARD_SOLAR_ADC_PIN                 (36)

    #define BOARD_SDA_PIN                       (21)
    #define BOARD_SCL_PIN                       (22)

    #define PRODUCT_MODEL_NAME                  "LilyGo-A7670 ESP32 Version"

#elif defined(LILYGO_T_CALL_A7670_V1_0)
    #define MODEM_BAUDRATE                      (115200)
    #define MODEM_DTR_PIN                       (14)
    #define MODEM_TX_PIN                        (26)
    #define MODEM_RX_PIN                        (25)
    #define BOARD_PWRKEY_PIN                    (4)
    #define BOARD_LED_PIN                       (12)
    #define LED_ON                              HIGH
    #define MODEM_RING_PIN                      (13)
    #define MODEM_RESET_PIN                     (27)
    #define MODEM_RESET_LEVEL                   LOW
    #define SerialAT                            Serial1

    #ifndef TINY_GSM_MODEM_A7670
        #define TINY_GSM_MODEM_A7670
    #endif

    #define PRODUCT_MODEL_NAME                  "LilyGo-T-Call A7670 V1.0"

#elif defined(LILYGO_T_CALL_A7670_V1_1)
    #define MODEM_BAUDRATE                      (115200)
    #define MODEM_DTR_PIN                       (32)
    #define MODEM_TX_PIN                        (27)
    #define MODEM_RX_PIN                        (26)
    #define BOARD_PWRKEY_PIN                    (4)
    #define BOARD_LED_PIN                       (13)
    #define LED_ON                              HIGH
    #define MODEM_RING_PIN                      (33)
    #define MODEM_RESET_PIN                     (5)
    #define MODEM_RESET_LEVEL                   LOW
    #define SerialAT                            Serial1

    #ifndef TINY_GSM_MODEM_A7670
        #define TINY_GSM_MODEM_A7670
    #endif

    #define PRODUCT_MODEL_NAME                  "LilyGo-T-Call A7670 V1.1"

#elif defined(LILYGO_T_SIM7670G_S3)
    #define MODEM_BAUDRATE                      (115200)
    #define MODEM_DTR_PIN                       (9)
    #define MODEM_TX_PIN                        (11)
    #define MODEM_RX_PIN                        (10)
    #define BOARD_PWRKEY_PIN                    (18)
    #define BOARD_LED_PIN                       (12)
    #define LED_ON                              (LOW)
    #define MODEM_RING_PIN                      (3)
    #define MODEM_RESET_PIN                     (17)
    #define MODEM_RESET_LEVEL                   LOW
    #define SerialAT                            Serial1

    #define BOARD_BAT_ADC_PIN                   (4)
    #define BOARD_SOLAR_ADC_PIN                 (5)
    #define BOARD_SDA_PIN                       (2)
    #define BOARD_SCL_PIN                       (1)

    #ifndef TINY_GSM_MODEM_SIM7670G
        #define TINY_GSM_MODEM_SIM7670G
    #endif

    #define PRODUCT_MODEL_NAME                  "LilyGo-SIM7670G-S3"

#elif defined(LILYGO_T_A7608X)
    #define MODEM_BAUDRATE                      (115200)
    #define MODEM_DTR_PIN                       (25)
    #define MODEM_TX_PIN                        (26)
    #define MODEM_RX_PIN                        (27)
    #define BOARD_PWRKEY_PIN                    (4)
    #define BOARD_POWERON_PIN                   (12)
    #define MODEM_RING_PIN                      (33)
    #define MODEM_RESET_PIN                     (5)
    #define BOARD_BAT_ADC_PIN                   (35)
    #define BOARD_SOLAR_ADC_PIN                 (34)
    #define MODEM_RESET_LEVEL                   HIGH
    #define SerialAT                            Serial1
    #define BOARD_SDA_PIN                       (21)
    #define BOARD_SCL_PIN                       (22)

    #ifndef TINY_GSM_MODEM_A7608
        #define TINY_GSM_MODEM_A7608
    #endif

    #define PRODUCT_MODEL_NAME                  "LilyGo-A7608X ESP32 Version"

#elif defined(LILYGO_T_A7608X_S3)
    #define MODEM_BAUDRATE                      (115200)
    #define MODEM_DTR_PIN                       (7)
    #define MODEM_TX_PIN                        (17)
    #define MODEM_RX_PIN                        (18)
    #define BOARD_PWRKEY_PIN                    (15)
    #define BOARD_BAT_ADC_PIN                   (4)
    #define BOARD_SOLAR_ADC_PIN                 (3)
    #define MODEM_RING_PIN                      (6)
    #define MODEM_RESET_PIN                     (16)
    #define MODEM_RESET_LEVEL                   LOW
    #define SerialAT                            Serial1
    #define BOARD_SDA_PIN                       (2)
    #define BOARD_SCL_PIN                       (1)

    #ifndef TINY_GSM_MODEM_A7608
        #define TINY_GSM_MODEM_A7608
    #endif

    #define PRODUCT_MODEL_NAME                  "LilyGo-A7608X-S3"

#elif defined(LILYGO_SIM7000G) || defined(LILYGO_SIM7070G)
    #define MODEM_DTR_PIN                       (25)
    #define MODEM_RX_PIN                        (26)
    #define MODEM_TX_PIN                        (27)
    #define BOARD_PWRKEY_PIN                    (4)
    #define BOARD_BAT_ADC_PIN                   (35)
    #define BOARD_SOLAR_ADC_PIN                 (36)
    #define SerialAT                            Serial1
    #define BOARD_SDA_PIN                       (21)
    #define BOARD_SCL_PIN                       (22)

    #ifdef LILYGO_SIM7000G
    #ifndef TINY_GSM_MODEM_SIM7000SSL
        #define TINY_GSM_MODEM_SIM7000SSL
    #endif
    #define PRODUCT_MODEL_NAME                  "LilyGo-SIM7000G ESP32 Version"
    #endif
    
    #ifdef LILYGO_SIM7070G
    #ifndef TINY_GSM_MODEM_SIM7080
        #define TINY_GSM_MODEM_SIM7080
    #endif
    #define PRODUCT_MODEL_NAME                  "LilyGo-SIM7070G ESP32 Version"
    #endif

#elif defined(LILYGO_SIM7600X)
    #define MODEM_DTR_PIN                       (32)
    #define MODEM_RX_PIN                        (26)
    #define MODEM_TX_PIN                        (27)
    #define BOARD_PWRKEY_PIN                    (4)
    #define MODEM_RING_PIN                      (33)
    #define BOARD_BAT_ADC_PIN                   (35)
    #define BOARD_SOLAR_ADC_PIN                 (36)
    #define SerialAT                            Serial1
    #define BOARD_SDA_PIN                       (21)
    #define BOARD_SCL_PIN                       (22)

    #ifndef TINY_GSM_MODEM_SIM7600
        #define TINY_GSM_MODEM_SIM7600
    #endif

    #define PRODUCT_MODEL_NAME                  "LilyGo-SIM7600X ESP32 Version"

#elif defined(LILYGO_SIM7000G_S3_STAN) || defined(LILYGO_SIM7080G_S3_STAN) \
    || defined(LILYGO_SIM7670G_S3_STAN) || defined(LILYGO_A7670X_S3_STAN)   \
    || defined(LILYGO_SIM7600X_S3_STAN)

    #define MODEM_DTR_PIN                       (7)
    #define MODEM_TX_PIN                        (4) 
    #define MODEM_RX_PIN                        (5) 
    #define BOARD_PWRKEY_PIN                    (46)
    #define BOARD_BAT_ADC_PIN                   (8)
    #define BOARD_SOLAR_ADC_PIN                 (18)
    #define SerialAT                            Serial1
    #define BOARD_SDA_PIN                       (3)
    #define BOARD_SCL_PIN                       (2)

    #ifdef LILYGO_SIM7000G_S3_STAN
    #ifndef TINY_GSM_MODEM_SIM7000SSL
        #define TINY_GSM_MODEM_SIM7000SSL
    #endif
    #endif

    #ifdef LILYGO_SIM7080G_S3_STAN
    #ifndef TINY_GSM_MODEM_SIM7080
        #define TINY_GSM_MODEM_SIM7080
    #endif
    #endif

    #ifdef LILYGO_SIM7670G_S3_STAN
    #ifndef TINY_GSM_MODEM_SIM7670G
        #define TINY_GSM_MODEM_SIM7670G
    #endif
    #endif

    #ifdef LILYGO_A7670X_S3_STAN
    #ifndef TINY_GSM_MODEM_A7670
        #define TINY_GSM_MODEM_A7670
    #endif
    #endif

    #ifdef LILYGO_SIM7600X_S3_STAN
    #ifndef TINY_GSM_MODEM_SIM7600
        #define TINY_GSM_MODEM_SIM7600
    #endif
    #endif

#else
    #error "Board selection mismatch. Check platformio.ini or utilities.h"
#endif

#ifndef MODEM_BAUDRATE
#define MODEM_BAUDRATE                      115200
#endif

#ifndef MODEM_RESET_LEVEL
#define MODEM_RESET_LEVEL                   LOW
#endif

// Power on/off pulse widths
#if defined(TINY_GSM_MODEM_A7670) || defined(TINY_GSM_MODEM_SIM7670G)
    #define MODEM_POWERON_PULSE_WIDTH_MS      (100)
    #define MODEM_POWEROFF_PULSE_WIDTH_MS     (3000)
#elif defined(TINY_GSM_MODEM_SIM7600)
    #define MODEM_POWERON_PULSE_WIDTH_MS      (500)
    #define MODEM_POWEROFF_PULSE_WIDTH_MS     (3000)
    #define MODEM_START_WAIT_MS               (15000)
#elif defined(TINY_GSM_MODEM_SIM7080) || defined(TINY_GSM_MODEM_SIM7000SSL)
    #define MODEM_POWERON_PULSE_WIDTH_MS      (1000)
    #define MODEM_POWEROFF_PULSE_WIDTH_MS     (1300)
#else
    #define MODEM_POWERON_PULSE_WIDTH_MS      1000
    #define MODEM_POWEROFF_PULSE_WIDTH_MS     3000
#endif

#ifndef MODEM_START_WAIT_MS
#define MODEM_START_WAIT_MS                 3000
#endif
