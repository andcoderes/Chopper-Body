#include "DomeController.h"
#include "../motor/MotorController.h"
#include "../web/logger.h"

volatile int32_t DomeController::encoderCount_ = 0;
volatile bool DomeController::homeTriggered_ = false;

// Guards encoderCount_, shared between onEncoderA() (ISR) and loop()/
// getEncoderCount() (Arduino task). Same portMUX_TYPE pattern used in
// EspNowController for its own ISR/task-shared state.
static portMUX_TYPE encoderMux_ = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR DomeController::onEncoderA() {
    if (digitalRead(PIN_DOME_ENC_B)) {
        encoderCount_--;
    } else {
        encoderCount_++;
    }
}

void IRAM_ATTR DomeController::onHomeTriggered() {
    homeTriggered_ = true;
}

void DomeController::setup(MotorController& motors) {
    motors_ = &motors;
    encoderCount_ = 0;
    homeTriggered_ = false;
    gearRatio_ = 0.0f;
    state_ = IDLE;

    pinMode(PIN_DOME_ENC_A, INPUT);
    pinMode(PIN_DOME_ENC_B, INPUT);
    pinMode(PIN_DOME_HOME, INPUT);

    attachInterrupt(digitalPinToInterrupt(PIN_DOME_ENC_A), onEncoderA, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_DOME_HOME), onHomeTriggered, FALLING);

    logAll("Dome encoder + home sensor initialized");
}

void DomeController::startCalibration() {
    if (state_ != IDLE) return;
    homeTriggered_ = false;
    state_ = SEEKING_HOME;
    calibrationStartTime_ = millis();
    // Speed is constant across SEEKING_HOME and ROTATING, so it only needs
    // to be written once here instead of every loop() iteration.
    motors_->setDome(DOME_CALIBRATION_SPEED);
    logAll("Dome calibration: seeking home");
}

bool DomeController::checkCalibrationTimeout(const char* phase) {
    if (millis() - calibrationStartTime_ < DOME_CALIBRATION_TIMEOUT_MS) return false;
    logAll("Dome calibration: timed out %s, aborting", phase);
    motors_->setDome(0);
    state_ = IDLE;
    return true;
}

void DomeController::loop() {
    switch (state_) {
        case IDLE:
            break;

        case SEEKING_HOME:
            if (checkCalibrationTimeout("seeking home")) break;
            if (homeTriggered_) {
                homeTriggered_ = false;
                portENTER_CRITICAL(&encoderMux_);
                encoderCount_ = 0;
                portEXIT_CRITICAL(&encoderMux_);
                state_ = ROTATING;
                calibrationStartTime_ = millis();  // fresh budget for the rotation phase
                logAll("Dome calibration: home found, counting");
            }
            break;

        case ROTATING:
            if (checkCalibrationTimeout("completing rotation")) break;
            if (homeTriggered_) {
                homeTriggered_ = false;
                portENTER_CRITICAL(&encoderMux_);
                int32_t count = encoderCount_;
                portEXIT_CRITICAL(&encoderMux_);
                gearRatio_ = (float)abs(count) / DOME_ENCODER_CPR;
                state_ = DONE;
            }
            break;

        case DONE:
            motors_->setDome(0);
            logAll("Dome calibration done: %ld ticks, ratio=%.2f",
                   (long)abs(encoderCount_), gearRatio_);
            state_ = IDLE;
            break;
    }
}

bool DomeController::isCalibrating() const {
    return state_ != IDLE;
}

float DomeController::getGearRatio() const {
    return gearRatio_;
}

int32_t DomeController::getEncoderCount() const {
    int32_t count;
    portENTER_CRITICAL(&encoderMux_);
    count = encoderCount_;
    portEXIT_CRITICAL(&encoderMux_);
    return count;
}

float DomeController::getRotations() const {
    return (float)getEncoderCount() / DOME_ENCODER_CPR;
}
