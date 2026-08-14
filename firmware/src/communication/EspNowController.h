#pragma once
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "MessageTypes.h"
#include "../config.h"

using CommandCallback = void (*)(const BodyCommand&);

class EspNowController {
public:
    void setup();
    void loop();
    void sendTelemetry();
    bool isConnected();
    void setCommandCallback(CommandCallback cb);
    void setMotorsActive(bool active) { motorsActive_ = active; }
    void setAudioPlaying(bool playing) { audioPlaying_ = playing; }

#ifdef UNIT_TEST
public:
#else
private:
#endif
    // IDF 5.x receive callback uses esp_now_recv_info_t
    static void onDataRecv(const esp_now_recv_info_t* info,
                           const uint8_t* data, int len);
    static void onDataSent(const esp_now_send_info_t* tx_info, esp_now_send_status_t status);

    CommandCallback callback_ = nullptr;
    unsigned long lastHeartbeat_ = 0;
    bool motorsActive_ = false;
    bool audioPlaying_ = false;

    static volatile bool dataReady_;
    static BodyCommand incomingBuffer_;
    static unsigned long lastRecvTime_;
    static volatile bool everReceived_;
};
