#pragma once

#include "Arduino.h"

class MiniMaestro {
public:
    MiniMaestro(HardwareSerial& serial) : serial_(serial) {}

    void restartScript(uint8_t scriptNum) {
        lastScriptRestarted = scriptNum;
        restartCallCount++;
    }

    void stopScript() {
        stopCallCount++;
    }

    void setTarget(uint8_t channel, uint16_t target) {
        lastChannel = channel;
        lastTarget = target;
    }

    // --- Inspection variables ---
    int lastScriptRestarted = -1;
    int restartCallCount = 0;
    int stopCallCount = 0;
    uint8_t lastChannel = 0;
    uint16_t lastTarget = 0;

private:
    HardwareSerial& serial_;
};
