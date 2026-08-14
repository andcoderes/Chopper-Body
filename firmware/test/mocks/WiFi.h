#pragma once

#include "Arduino.h"

class WiFiClass {
public:
    void mode(int) {}
    void disconnect() {}
    String macAddress() { return String("AA:BB:CC:DD:EE:FF"); }
};

extern WiFiClass WiFi;
