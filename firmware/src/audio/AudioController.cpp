#include "AudioController.h"
#include "../web/logger.h"

void AudioController::setup() {
    Serial2.begin(9600, SERIAL_8N1, PIN_MP3_RX, PIN_MP3_TX);
    delay(500);
    sendCommand(0x09, 0x00, 0x02);  // select TF/SD card as storage device (required after power-up)
    delay(200);
    setVolume(15);
    logAll("Audio initialized");
}

void AudioController::loop() {
    while (Serial2.available()) {
        uint8_t b = Serial2.read();
        logAll("Audio RX: 0x%02X", b);
        feedRxByte(b);
    }
}

// Feeds one byte of an incoming response frame through a position state
// machine. On a complete 0x3D frame (module-reported "playback finished"),
// clears playing_ so isPlaying()/telemetry reflect reality instead of
// staying stuck true until the next playTrack()/pause() call.
void AudioController::feedRxByte(uint8_t b) {
    switch (rxState_) {
        case 0: rxState_ = (b == 0x7E) ? 1 : 0; break;
        case 1: rxState_ = (b == 0xFF) ? 2 : 0; break;
        case 2: rxState_ = (b == 0x06) ? 3 : 0; break;
        case 3: rxCmd_ = b; rxState_ = 4; break;
        case 4: rxState_ = 5; break;  // feedback byte, ignored
        case 5: rxState_ = 6; break;  // dataH, ignored
        case 6: rxState_ = 7; break;  // dataL, ignored
        case 7:
            if (b == 0xEF && rxCmd_ == 0x3D) {
                playing_ = false;
            }
            rxState_ = 0;
            break;
        default: rxState_ = 0; break;
    }
}

void AudioController::sendCommand(uint8_t cmd, uint8_t dataH, uint8_t dataL) {
    uint8_t frame[8] = {0x7E, 0xFF, 0x06, cmd, 0x00, dataH, dataL, 0xEF};
    Serial2.write(frame, sizeof(frame));
}

void AudioController::playTrack(uint8_t track) {
    if (track == 0) return;
    logAll("Audio: play track %d", track);
    sendCommand(0x03, 0x00, track);
    playing_ = true;
}

void AudioController::setVolume(uint8_t vol) {
    vol = constrain(vol, 0, 30);
    if (vol == currentVolume_) return;
    currentVolume_ = vol;
    logAll("Audio: volume %d", vol);
    sendCommand(0x06, 0x00, vol);
}

void AudioController::pause() {
    sendCommand(0x0E, 0x00, 0x00);
    playing_ = false;
}

void AudioController::resume() {
    sendCommand(0x0D, 0x00, 0x00);
    playing_ = true;
}

bool AudioController::isPlaying() const {
    return playing_;
}
