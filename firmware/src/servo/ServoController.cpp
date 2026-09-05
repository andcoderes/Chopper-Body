#include "ServoController.h"
#include "../web/logger.h"

void ServoController::setup() {
    Serial1.begin(9600, SERIAL_8N1, PIN_MAESTRO_RX, PIN_MAESTRO_TX);
    delay(500);
}

bool ServoController::isReady() const {
    return ready_;
}

bool ServoController::checkConnection() {
    if (ready_) return true;
    if (checkTimedOut_) return false;

    if (!checkStarted_) {
        checkStarted_ = true;
        checkStartTime_ = millis();
    }
    if (millis() - checkStartTime_ >= CONNECT_TIMEOUT_MS) {
        checkTimedOut_ = true;
        logAll("Maestro: connect timed out after 3s, giving up");
        return false;
    }

    if (!checkPending_) {
        while (Serial1.available()) Serial1.read();
        if (millis() - lastConnectLogTime_ >= 1000) {
            logAll("Maestro: trying to connect...");
            lastConnectLogTime_ = millis();
        }
        Serial1.write(0xA1);
        checkPending_ = true;
        checkSentTime_ = millis();
        return false;
    }

    if (Serial1.available() >= 2) {
        Serial1.read();
        Serial1.read();
        ready_ = true;
        logAll("Maestro: connected");
        return true;
    }

    if (millis() - checkSentTime_ >= 100) {
        checkPending_ = false;
    }

    return false;
}

void ServoController::stop() {
    maestro_.stopScript();
}

void ServoController::animate(const char* button, const int16_t macros[], int macroCount) {
    String key;
    if (button != nullptr && button[0] != '\0') {
        key = button;
    } else if (macroCount > 0 && macros[0] != 0) {
        key = String(macros[0]);
    } else {
        return;
    }

    logAll("Servo: key=%s", key.c_str());
    playScriptButton(key);
}

void ServoController::playScriptButton(const String& keyAction) {
    int index = -1;
    for (int i = 0; i < NUM_KEYS; i++) {
        if (arrayKeys_[i] == keyAction) {
            index = i;
            break;
        }
    }
    if (index == -1) return;

    int scriptToRun = scripts_[index][currentIndex_[index]];
    currentIndex_[index] = (currentIndex_[index] + 1) % scriptLen_[index];

    maestro_.stopScript();
    logAll("Servo: script %d", scriptToRun);

    if (scriptToRun == 100) {
        maestro_.setTarget(3, 5544);
        maestro_.setTarget(5, 2000);
    } else {
        maestro_.setTarget(5, 0);
        maestro_.restartScript((uint8_t)scriptToRun);
    }
}
