#include <unity.h>
#include "audio/AudioController.h"

static AudioController* ac;

void setUp(void) {
    mock_millis_value = 0;
    Serial2.resetMock();
    ac = new AudioController();
    // Don't call setup() here to avoid side effects; test raw methods
}

void tearDown(void) {
    delete ac;
}

// --- playTrack ---
void test_playTrack_sends_correct_frame(void) {
    Serial2.resetMock();
    ac->playTrack(5);
    // Expected: {0x7E, 0xFF, 0x06, 0x03, 0x00, 0x00, 0x05, 0xEF}
    TEST_ASSERT_EQUAL(1, Serial2.writeCallCount);
    TEST_ASSERT_EQUAL_HEX8(0x7E, Serial2.writtenBytes[0]);
    TEST_ASSERT_EQUAL_HEX8(0xFF, Serial2.writtenBytes[1]);
    TEST_ASSERT_EQUAL_HEX8(0x06, Serial2.writtenBytes[2]);
    TEST_ASSERT_EQUAL_HEX8(0x03, Serial2.writtenBytes[3]);  // play by index cmd
    TEST_ASSERT_EQUAL_HEX8(0x00, Serial2.writtenBytes[4]);
    TEST_ASSERT_EQUAL_HEX8(0x00, Serial2.writtenBytes[5]);
    TEST_ASSERT_EQUAL_HEX8(0x05, Serial2.writtenBytes[6]);  // track 5
    TEST_ASSERT_EQUAL_HEX8(0xEF, Serial2.writtenBytes[7]);
}

void test_playTrack_zero_is_ignored(void) {
    Serial2.resetMock();
    ac->playTrack(0);
    TEST_ASSERT_EQUAL(0, Serial2.writeCallCount);
}

void test_playTrack_sets_playing(void) {
    TEST_ASSERT_FALSE(ac->isPlaying());
    ac->playTrack(1);
    TEST_ASSERT_TRUE(ac->isPlaying());
}

void test_playTrack_max_value(void) {
    Serial2.resetMock();
    ac->playTrack(255);
    TEST_ASSERT_EQUAL_HEX8(0xFF, Serial2.writtenBytes[6]);  // track 255
}

// --- setVolume ---
void test_setVolume_sends_correct_frame(void) {
    Serial2.resetMock();
    ac->setVolume(20);
    // Expected: {0x7E, 0xFF, 0x06, 0x06, 0x00, 0x00, 0x14, 0xEF}
    TEST_ASSERT_EQUAL(1, Serial2.writeCallCount);
    TEST_ASSERT_EQUAL_HEX8(0x06, Serial2.writtenBytes[3]);  // set volume cmd
    TEST_ASSERT_EQUAL_HEX8(0x14, Serial2.writtenBytes[6]);  // volume 20
}

void test_setVolume_skips_if_same(void) {
    Serial2.resetMock();
    ac->setVolume(20);
    int countAfterFirst = Serial2.writeCallCount;

    Serial2.resetMock();
    ac->setVolume(20);
    TEST_ASSERT_EQUAL(0, Serial2.writeCallCount);
}

void test_setVolume_clamps_to_30(void) {
    Serial2.resetMock();
    ac->setVolume(50);  // clamped to 30
    TEST_ASSERT_EQUAL_HEX8(0x1E, Serial2.writtenBytes[6]);  // 30 = 0x1E
}

void test_setVolume_zero(void) {
    Serial2.resetMock();
    ac->setVolume(0);
    TEST_ASSERT_EQUAL_HEX8(0x00, Serial2.writtenBytes[6]);
}

// --- pause / resume ---
void test_pause_sends_command_and_clears_playing(void) {
    ac->playTrack(1);
    TEST_ASSERT_TRUE(ac->isPlaying());

    Serial2.resetMock();
    ac->pause();
    TEST_ASSERT_FALSE(ac->isPlaying());
    TEST_ASSERT_EQUAL_HEX8(0x0E, Serial2.writtenBytes[3]);  // pause cmd
}

void test_resume_sends_command_and_sets_playing(void) {
    Serial2.resetMock();
    ac->resume();
    TEST_ASSERT_TRUE(ac->isPlaying());
    TEST_ASSERT_EQUAL_HEX8(0x0D, Serial2.writtenBytes[3]);  // resume cmd
}

// --- module-reported playback finished (0x3D frame) ---
static void feedFrame(AudioController* ac, uint8_t cmd) {
    const uint8_t frame[8] = {0x7E, 0xFF, 0x06, cmd, 0x00, 0x00, 0x00, 0xEF};
    for (uint8_t b : frame) ac->feedRxByte(b);
}

void test_finished_frame_clears_playing(void) {
    ac->playTrack(1);
    TEST_ASSERT_TRUE(ac->isPlaying());

    feedFrame(ac, 0x3D);
    TEST_ASSERT_FALSE(ac->isPlaying());
}

void test_unrelated_frame_does_not_clear_playing(void) {
    ac->playTrack(1);
    TEST_ASSERT_TRUE(ac->isPlaying());

    feedFrame(ac, 0x41);  // ack, not "finished"
    TEST_ASSERT_TRUE(ac->isPlaying());
}

void test_malformed_finished_frame_does_not_clear_playing(void) {
    ac->playTrack(1);
    TEST_ASSERT_TRUE(ac->isPlaying());

    // Right cmd byte, but wrong footer byte in the last position — must
    // not be mistaken for a valid "finished" frame.
    const uint8_t badFrame[8] = {0x7E, 0xFF, 0x06, 0x3D, 0x00, 0x00, 0x00, 0x99};
    for (uint8_t b : badFrame) ac->feedRxByte(b);

    TEST_ASSERT_TRUE(ac->isPlaying());
}

// --- frame structure ---
void test_all_frames_have_correct_header_and_footer(void) {
    Serial2.resetMock();
    ac->pause();
    TEST_ASSERT_EQUAL_HEX8(0x7E, Serial2.writtenBytes[0]);  // header
    TEST_ASSERT_EQUAL_HEX8(0xFF, Serial2.writtenBytes[1]);  // version
    TEST_ASSERT_EQUAL_HEX8(0x06, Serial2.writtenBytes[2]);  // length
    TEST_ASSERT_EQUAL_HEX8(0x00, Serial2.writtenBytes[4]);  // feedback (off)
    TEST_ASSERT_EQUAL_HEX8(0xEF, Serial2.writtenBytes[7]);  // footer
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_playTrack_sends_correct_frame);
    RUN_TEST(test_playTrack_zero_is_ignored);
    RUN_TEST(test_playTrack_sets_playing);
    RUN_TEST(test_playTrack_max_value);
    RUN_TEST(test_setVolume_sends_correct_frame);
    RUN_TEST(test_setVolume_skips_if_same);
    RUN_TEST(test_setVolume_clamps_to_30);
    RUN_TEST(test_setVolume_zero);
    RUN_TEST(test_pause_sends_command_and_clears_playing);
    RUN_TEST(test_resume_sends_command_and_sets_playing);
    RUN_TEST(test_all_frames_have_correct_header_and_footer);
    RUN_TEST(test_finished_frame_clears_playing);
    RUN_TEST(test_unrelated_frame_does_not_clear_playing);
    RUN_TEST(test_malformed_finished_frame_does_not_clear_playing);
    UNITY_END();
    return 0;
}
