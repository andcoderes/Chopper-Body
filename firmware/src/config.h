#pragma once
#include <Arduino.h>

// --- MDDS30 drive motor pins (PWM Independent mode, DIP: 0b10110100) ---
// External pull-down resistors required on all 4 pins: while the ESP32 is
// powering on/off (or resetting) these GPIOs float/default HIGH before
// setup() runs pinMode()/ledcAttachPin(), which the MDDS30 can read as a
// drive command and briefly engage the motors.
#define PIN_MDDS_IN1    13   // Left motor direction (LOW=fwd, HIGH=rev)
#define PIN_MDDS_AN1    27   // Left motor speed (LEDC PWM, 5kHz 8-bit)
#define PIN_MDDS_IN2    12   // Right motor direction (LOW=fwd, HIGH=rev)
#define PIN_MDDS_AN2    14   // Right motor speed (LEDC PWM, 5kHz 8-bit)

// --- Dome encoder + home sensor (top of left side, pads 6-7-8) ---
#define PIN_DOME_ENC_A  35   // Encoder channel A (input-only, 10k+20k divider)
#define PIN_DOME_ENC_B  34   // Encoder channel B (input-only, 10k+20k divider)
#define PIN_DOME_HOME   32   // A3144 hall-effect home sensor (HiLetgo module, 10k+20k divider)
#define DOME_ENCODER_CPR 3200 // Counts per motor output shaft revolution (64 CPR * 50:1)
#define DOME_CALIBRATION_SPEED 40 // PWM % for calibration spin
#define DOME_CALIBRATION_TIMEOUT_MS 15000 // Abort calibration if home sensor never fires (safety cutoff)

// --- Cytron MD10C dome rotation motor pins (pads 9-10, adjacent below encoder) ---
// External 10k pull-down resistors required on both pins (same reason as MDDS30).
#define PIN_DOME_DIR    33   // Dome direction (LOW=fwd, HIGH=rev)
#define PIN_DOME_PWM    25   // Dome speed (LEDC PWM, 5kHz 8-bit)

// --- Pololu Maestro servo controller (Serial1) ---
#define PIN_MAESTRO_TX  23   // Serial1 TX -> Maestro RX (9600 baud)
#define PIN_MAESTRO_RX  22   // Serial1 RX <- Maestro TX (1k+2k divider)

// --- MP3 player (Serial2) ---
#define PIN_MP3_TX       4   // Serial2 TX -> MP3 RX (9600 baud)
#define PIN_MP3_RX      16   // Serial2 RX <- MP3 TX (1k+2k divider)

// --- Bubbles relay (pad 11, below MD10C) ---
#define PIN_BUBBLES     26   // On/off relay module

// --- Secrets (ESP-NOW keys, controller MAC, WiFi AP password) ---
#ifdef UNIT_TEST
// Dummy values for native unit tests — no real ESP-NOW traffic or AP off-device.
static const uint8_t PMK_KEY[16] = {0};
static const uint8_t LMK_KEY[16] = {0};
static const uint8_t CONTROLLER_MAC[6] = {0};
#define WIFI_AP_PASSWORD "test"
#else
#include "secrets.h"  // defines PMK_KEY, LMK_KEY, CONTROLLER_MAC, WIFI_AP_PASSWORD — generated from .env, see README
#endif

// --- Timing constants ---
#define HEARTBEAT_INTERVAL_MS   1000   // How often to send telemetry
#define CONNECTION_TIMEOUT_MS   5000   // No data = disconnected
#define MOTOR_TIMEOUT_MS         500   // Stop motors if no command
#define AP_START_TIMEOUT_MS     3000   // Start AP if mesh not connected by this long after boot

// --- WiFi AP for web server ---
#define WIFI_AP_SSID     "ChopperBody"
// WIFI_AP_PASSWORD comes from secrets.h (generated from .env, see README)
