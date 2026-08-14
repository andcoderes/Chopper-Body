#pragma once
#include <Arduino.h>
#include <PololuMaestro.h>
#include "../config.h"

// Body servo scripts on the Maestro (placeholder — configure for your build):
// 0 = script 0
// 1 = script 1
// ...

class ServoController {
public:
    void setup();
    void animate(const char* button, const int16_t macros[], int macroCount);
    void execute(int script);
    void stop();
    bool isReady() const;
    bool checkConnection();

#ifdef UNIT_TEST
public:
#else
private:
#endif
    MiniMaestro maestro_{Serial1};

    static const int NUM_KEYS = 7;
    String arrayKeys_[NUM_KEYS] = {"y", "x", "b", "du", "dd", "dr", "dl"};
    int currentIndex_[NUM_KEYS] = {0, 0, 0, 0, 0, 0, 0};
    int scripts_[NUM_KEYS][2] = {{0,1}, {2,2}, {3,3}, {4,4}, {5,5}, {6,6}, {6,6}};

    bool handOut_ = false;
    int animationRequiredHandOut_[3] = {1, 2, 3};
    int outHandAnimation_ = 0;
    int inHandAnimation_ = 1;

    bool ready_ = false;
    bool checkPending_ = false;
    bool checkStarted_ = false;
    bool checkTimedOut_ = false;
    unsigned long checkSentTime_ = 0;
    unsigned long checkStartTime_ = 0;
    unsigned long lastConnectLogTime_ = 0;

    static const unsigned long CONNECT_TIMEOUT_MS = 3000;

    void playScriptButton(const String& keysAction);
    bool checkAnimation(int animation);
};
