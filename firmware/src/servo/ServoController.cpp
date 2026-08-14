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
        // Flush any stale data
        while (Serial1.available()) Serial1.read();
        // Send getErrors compact protocol command
        if (millis() - lastConnectLogTime_ >= 1000) {
            logAll("Maestro: trying to connect...");
            lastConnectLogTime_ = millis();
        }
        Serial1.write(0xA1);
        checkPending_ = true;
        checkSentTime_ = millis();
        return false;
    }

    // Waiting for response
    if (Serial1.available() >= 2) {
        Serial1.read();
        Serial1.read();
        ready_ = true;
        logAll("Maestro: connected");
        return true;
    }

    // Timeout after 100ms — retry next loop
    if (millis() - checkSentTime_ >= 100) {
        checkPending_ = false;
    }

    return false;
}

void ServoController::stop() {
    maestro_.stopScript();
}

void ServoController::animate(const char* button, const int16_t macros[], int macroCount) {
    // Try button first
    if (button[0] != '\0') {
        logAll("Servo: btn=%s", button);
        playScriptButton(String(button));
        return;
    }

    // Try macros - build a key string from the macro values
    String macrosString = "";
    for (int i = 0; i < macroCount && macros[i] != 0; i++) {
        macrosString += String(macros[i]);
    }

    if (macrosString != "") {
        logAll("Servo: macro=%s", macrosString.c_str());
        playScriptButton(macrosString);
    }
}

void ServoController::playScriptButton(const String& keysAction) {
    int actionIndex = -1;
    for (int i = 0; i < NUM_KEYS; i++) {
        if (arrayKeys_[i] == keysAction) {
            actionIndex = i;
            break;
        }
    }

    if (actionIndex != -1) {
        int currentIndex = currentIndex_[actionIndex];
        int scriptToRun = scripts_[actionIndex][currentIndex];
        currentIndex = (currentIndex + 1) % 2;
        currentIndex_[actionIndex] = currentIndex;

        if (checkAnimation(scriptToRun)) {
            logAll("Servo: restart script %d", scriptToRun);
            maestro_.restartScript(scriptToRun);
        }
    }
}

void ServoController::execute(int scriptToRun) {
    if (checkAnimation(scriptToRun)) {
        maestro_.restartScript(scriptToRun);
    }
}

bool ServoController::checkAnimation(int animation) {
    if (animation == outHandAnimation_) {
        handOut_ = true;
        return true;
    }
    if (handOut_) {
        if (animation == inHandAnimation_) {
            handOut_ = false;
            return true;
        }
        for (int x = 0; x < 3; x++) {
            if (animationRequiredHandOut_[x] == animation) {
                return true;
            }
        }
    }
    if (animation > 3) {
        return true;
    }
    return false;
}
