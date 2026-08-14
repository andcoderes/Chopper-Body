#pragma once
#include <Arduino.h>
#include "../config.h"

class MotorController;

class DomeController {
public:
    void setup(MotorController& motors);
    void startCalibration();
    void loop();
    bool isCalibrating() const;
    float getGearRatio() const;
    int32_t getEncoderCount() const;
    float getRotations() const;

private:
    static void IRAM_ATTR onEncoderA();
    static void IRAM_ATTR onHomeTriggered();
    // Returns true (and stops+aborts) if the current calibration phase has
    // run longer than DOME_CALIBRATION_TIMEOUT_MS. Shared by SEEKING_HOME
    // and ROTATING, which otherwise duplicate this check verbatim.
    bool checkCalibrationTimeout(const char* phase);

    static volatile int32_t encoderCount_;
    static volatile bool homeTriggered_;

    MotorController* motors_ = nullptr;
    float gearRatio_ = 0.0f;
    enum State { IDLE, SEEKING_HOME, ROTATING, DONE };
    State state_ = IDLE;
    // Safety cutoff: if the home sensor never fires (miswired, unpowered,
    // misaligned magnet), the dome must not spin indefinitely.
    unsigned long calibrationStartTime_ = 0;
};
