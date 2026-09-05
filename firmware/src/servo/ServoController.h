#pragma once
#include <Arduino.h>
#include <PololuMaestro.h>
#include "../config.h"

class ServoController {
public:
    void setup();
    void animate(const char* button, const int16_t macros[], int macroCount);
    void stop();
    bool isReady() const;
    bool checkConnection();

#ifdef UNIT_TEST
public:
#else
private:
#endif
    MiniMaestro maestro_{Serial1};

    static const int NUM_KEYS    = 5;
    static const int MAX_SCRIPTS = 3;

    // script 7 = crazy body, script 8 = bubble head
    String arrayKeys_[NUM_KEYS] = {"l1", "r1", "a", "102", "107"};
    int scripts_[NUM_KEYS][MAX_SCRIPTS] = {
        {0,   1,   2},
        {3, 100,   4},
        {5,   6,   6},
        {7,   7,   7},   // key "102" -> crazy body
        {8,   8,   8},   // key "107" -> bubble head
    };
    uint8_t scriptLen_[NUM_KEYS]   = {3, 3, 2, 1, 1};
    int     currentIndex_[NUM_KEYS] = {0, 0, 0, 0, 0};

    bool ready_ = false;
    bool checkPending_ = false;
    bool checkStarted_ = false;
    bool checkTimedOut_ = false;
    unsigned long checkSentTime_ = 0;
    unsigned long checkStartTime_ = 0;
    unsigned long lastConnectLogTime_ = 0;

    static const unsigned long CONNECT_TIMEOUT_MS = 3000;

    void playScriptButton(const String& keyAction);
};
