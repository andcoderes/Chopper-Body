#pragma once
#include <Arduino.h>

// Status codes
#define STATUS_BUTTONS     0
#define STATUS_MOVEMENT    1
#define STATUS_SETTINGS    2
#define STATUS_AP_CONTROL  3
#define STATUS_CONNECTION -1

// --- Controller -> Body (msgType=3) ---
struct BodyCommand {
    int8_t  msgType;           // always 3
    int8_t  status;            // 0=buttons, 1=movement, 2=settings, -1=connection
    int8_t  lx;                // drive X: -100 to 100
    int8_t  ly;                // drive Y: -100 to 100
    int8_t  domeSpeed;         // dome rotation: -100 to 100
    uint8_t audioTrack;        // 0=none, 1-255=play track
    uint8_t volume;            // 0-30
    uint8_t bubbles;           // 0=off, 1=on
    char    button[8];         // servo button (e.g. "y", "du")
    int16_t macro[4];          // servo macro IDs
    int8_t  connectionStatus;
    uint8_t apRequested;       // STATUS_AP_CONTROL: 1=start AP, 0=no-op
} __attribute__((packed));     // 26 bytes

// --- Body -> Controller (msgType=4) ---
struct BodyTelemetry {
    int8_t  msgType;           // always 4
    uint8_t connected;
    uint8_t motorsActive;
    uint8_t audioPlaying;
    uint32_t uptimeMs;
} __attribute__((packed));     // 8 bytes
