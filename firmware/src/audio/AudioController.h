#pragma once
#include <Arduino.h>
#include "../config.h"

class AudioController {
public:
    void setup();
    void loop();
    void playTrack(uint8_t track);
    void setVolume(uint8_t vol);
    void pause();
    void resume();
    bool isPlaying() const;

#ifdef UNIT_TEST
public:
#else
private:
#endif
    void feedRxByte(uint8_t b);

private:
    void sendCommand(uint8_t cmd, uint8_t dataH, uint8_t dataL);
    bool playing_ = false;
    uint8_t currentVolume_ = 15;
    // Small state machine tracking position within an incoming 8-byte
    // response frame (0x7E 0xFF 0x06 CMD 0x00 dataH dataL 0xEF), so we can
    // notice a 0x3D "playback finished" frame and clear playing_.
    uint8_t rxState_ = 0;
    uint8_t rxCmd_ = 0;
};
