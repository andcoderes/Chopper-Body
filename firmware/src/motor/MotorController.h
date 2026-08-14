#pragma once
#include <Arduino.h>
#include "../config.h"

class MotorController {
public:
    void setup();
    void setDrive(int8_t lx, int8_t ly);
    void setDome(int8_t speed);
    void stopAll();
    bool isActive() const;
    void checkTimeout();

private:
    void setMotor(uint8_t dirPin, uint8_t pwmPin, int speed);
    unsigned long lastCommandTime_ = 0;
    // Tracked separately so the dome motor is covered by the same
    // dead-man's-switch timeout as the drive wheels (setDome() previously
    // never marked anything active, so checkTimeout() never stopped a
    // dome-only command).
    bool driveActive_ = false;
    bool domeActive_ = false;
};
