#ifndef UNIT_TEST

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <cstring>
#include "config.h"
#include "communication/EspNowController.h"
#include "communication/MessageTypes.h"
#include "motor/MotorController.h"
#include "dome/DomeController.h"
#include "servo/ServoController.h"
#include "audio/AudioController.h"
#include "web/WebController.h"
#include "web/logger.h"

EspNowController espNow;
MotorController motors;
DomeController dome;
ServoController servo;
AudioController audio;
WebController webController;

bool apActive_ = false;
bool apStartRequested_ = false;
unsigned long bootTime_ = 0;

// Starts the AP + web/captive-portal server. Off by default; only called
// once one of the AP-start conditions is met (see loop()).
void startAP() {
    if (apActive_) return;

    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
    WiFi.softAPConfig(IPAddress(192,168,5,1), IPAddress(192,168,5,1), IPAddress(255,255,255,0));

    webController.setDomeController(&dome);
    webController.setup();

    apActive_ = true;
    logAll("AP started, IP: %s", WiFi.softAPIP().toString().c_str());
}

void onCommand(const BodyCommand& cmd) {
    switch (cmd.status) {
        case STATUS_MOVEMENT:
            logAll("ESP-NOW RX: move lx=%d ly=%d dome=%d", cmd.lx, cmd.ly, cmd.domeSpeed);
            motors.setDrive(cmd.lx, cmd.ly);
            // Dome calibration drives the dome motor itself via dome.loop();
            // an RF movement command here would fight that write.
            if (!dome.isCalibrating()) {
                motors.setDome(cmd.domeSpeed);
            }
            break;

        case STATUS_BUTTONS: {
            logAll("ESP-NOW RX: button='%s' macro=[%d,%d,%d,%d]",
                   cmd.button, cmd.macro[0], cmd.macro[1], cmd.macro[2], cmd.macro[3]);
            int16_t macro[4];
            memcpy(macro, cmd.macro, sizeof(macro));
            servo.animate(cmd.button, macro, 4);
            break;
        }

        case STATUS_CONNECTION:
            logAll("ESP-NOW RX: connection status=%d", cmd.connectionStatus);
            if (cmd.connectionStatus == 0) {
                motors.stopAll();
            }
            break;

        case STATUS_SETTINGS:
            logAll("ESP-NOW RX: settings track=%u vol=%u bubbles=%u",
                   cmd.audioTrack, cmd.volume, cmd.bubbles);
            audio.setVolume(cmd.volume);
            break;

        case STATUS_AP_CONTROL:
            logAll("ESP-NOW RX: AP control requested=%u", cmd.apRequested);
            if (cmd.apRequested) {
                apStartRequested_ = true;
            }
            break;

        default:
            logAll("ESP-NOW RX: unknown status=%d", cmd.status);
            break;
    }

    // Audio and bubbles are handled regardless of status
    if (cmd.audioTrack > 0) {
        audio.playTrack(cmd.audioTrack);
    }
    // cmd.bubbles is the current toggle state from the receiver (see
    // CommandParser), re-stamped onto every packet — just mirror it.
    if (cmd.bubbles) {
        logAll("starting bubbles");
    } else {
        logAll("bubble off");
    }
    digitalWrite(PIN_BUBBLES, cmd.bubbles ? HIGH : LOW);
}

void setup() {
    bootTime_ = millis();

    Serial.begin(115200);
    delay(500);

    // Bubbles relay
    pinMode(PIN_BUBBLES, OUTPUT);
    digitalWrite(PIN_BUBBLES, LOW);

    motors.setup();
    dome.setup(motors);
    servo.setup();
    audio.setup();

    // WiFi: AP+STA mode for ESP-NOW + web server coexistence.
    // AP itself stays off (WiFi.softAP not called) until an AP-start
    // condition is met in loop() — see startAP().
    WiFi.mode(WIFI_AP_STA);
    esp_wifi_set_ps(WIFI_PS_NONE);

    espNow.setup();
    espNow.setCommandCallback(onCommand);

    logAll("Chopper V2 Body ready");
}

void loop() {
    // Safety: stop motors on timeout or ESP-NOW disconnect
    motors.checkTimeout();
    if (!espNow.isConnected()) {
        motors.stopAll();
    }

    // Dome encoder / calibration
    dome.loop();

    // Update telemetry state
    espNow.setMotorsActive(motors.isActive());
    espNow.setAudioPlaying(audio.isPlaying());

    // Process ESP-NOW messages and send heartbeat
    espNow.loop();

    // Log any bytes the MP3 module sends back (diagnostics)
    audio.loop();

    // Servo connection check
    if (!servo.isReady()) {
        servo.checkConnection();
    }

    // AP is off by default. Start it if:
    //  - the mesh (ESP-NOW) link hasn't connected within AP_START_TIMEOUT_MS
    //    of boot, or
    //  - a peer over the mesh explicitly requested it (STATUS_AP_CONTROL)
    if (!apActive_) {
        bool meshTimedOut = (millis() - bootTime_ >= AP_START_TIMEOUT_MS) && !espNow.isConnected();
        if (apStartRequested_ || meshTimedOut) {
            startAP();
        }
    }

    // Web server (only serving once the AP is up)
    if (apActive_) {
        webController.loop();
    }
}

#endif // UNIT_TEST
