#include "MotorController.h"
#include "../web/logger.h"

void MotorController::setup() {
    // MDDS30 direction pins
    pinMode(PIN_MDDS_IN1, OUTPUT);
    pinMode(PIN_MDDS_IN2, OUTPUT);

    // MDDS30 PWM speed channels (5kHz, 8-bit resolution)
    ledcAttach(PIN_MDDS_AN1, 5000, 8);
    ledcAttach(PIN_MDDS_AN2, 5000, 8);

    // MD10C dome direction pin
    pinMode(PIN_DOME_DIR, OUTPUT);

    // MD10C dome PWM speed channel (5kHz, 8-bit resolution)
    ledcAttach(PIN_DOME_PWM, 5000, 8);

    stopAll();
    logAll("Motors initialized");
}

void MotorController::setMotor(uint8_t dirPin, uint8_t pwmPin, int speed) {
    speed = constrain(speed, -100, 100);
    if (speed == 0) {
        digitalWrite(dirPin, LOW);
        ledcWrite(pwmPin, 0);
    } else {
        digitalWrite(dirPin, speed < 0 ? HIGH : LOW);
        ledcWrite(pwmPin, (uint32_t)(abs(speed) * 255 / 100));
    }
}

void MotorController::setDrive(int8_t lx, int8_t ly) {
    // Differential drive mixing
    int left  = constrain((int)ly + (int)lx, -100, 100);
    int right = constrain((int)ly - (int)lx, -100, 100);

    setMotor(PIN_MDDS_IN1, PIN_MDDS_AN1, left);
    setMotor(PIN_MDDS_IN2, PIN_MDDS_AN2, right);

    driveActive_ = (left != 0 || right != 0);
    lastCommandTime_ = millis();
}

void MotorController::setDome(int8_t speed) {
    setMotor(PIN_DOME_DIR, PIN_DOME_PWM, speed);
    domeActive_ = (speed != 0);
    lastCommandTime_ = millis();
}

void MotorController::stopAll() {
    setMotor(PIN_MDDS_IN1, PIN_MDDS_AN1, 0);
    setMotor(PIN_MDDS_IN2, PIN_MDDS_AN2, 0);
    setMotor(PIN_DOME_DIR, PIN_DOME_PWM, 0);
    driveActive_ = false;
    domeActive_ = false;
}

bool MotorController::isActive() const {
    return driveActive_ || domeActive_;
}

void MotorController::checkTimeout() {
    if ((driveActive_ || domeActive_) && (millis() - lastCommandTime_ >= MOTOR_TIMEOUT_MS)) {
        logAll("Motor timeout - stopping");
        stopAll();
    }
}
