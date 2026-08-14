#include <unity.h>
#include "communication/MessageTypes.h"

void setUp(void) {}
void tearDown(void) {}

// --- Struct sizes ---
void test_BodyCommand_size_is_26_bytes(void) {
    // int8_t(1) + int8_t(1) + int8_t(1) + int8_t(1) + int8_t(1)
    // + uint8_t(1) + uint8_t(1) + uint8_t(1) + char[8](8) + int16_t[4](8)
    // + int8_t(1) + uint8_t(1) = 26
    TEST_ASSERT_EQUAL(26, sizeof(BodyCommand));
}

void test_BodyTelemetry_size_is_8_bytes(void) {
    // int8_t(1) + uint8_t(1) + uint8_t(1) + uint8_t(1) + uint32_t(4) = 8
    TEST_ASSERT_EQUAL(8, sizeof(BodyTelemetry));
}

// --- Field copy ---
void test_BodyCommand_field_copy(void) {
    BodyCommand cmd = {};
    cmd.msgType = 3;
    cmd.status  = STATUS_MOVEMENT;
    cmd.lx = -50;
    cmd.ly = 75;
    cmd.domeSpeed = -30;
    cmd.audioTrack = 5;
    cmd.volume = 20;
    cmd.bubbles = 1;
    strncpy(cmd.button, "du", sizeof(cmd.button));
    cmd.macro[0] = 101;
    cmd.macro[1] = 103;
    cmd.connectionStatus = 1;

    BodyCommand copy;
    memcpy(&copy, &cmd, sizeof(BodyCommand));

    TEST_ASSERT_EQUAL_INT8(3, copy.msgType);
    TEST_ASSERT_EQUAL_INT8(STATUS_MOVEMENT, copy.status);
    TEST_ASSERT_EQUAL_INT8(-50, copy.lx);
    TEST_ASSERT_EQUAL_INT8(75, copy.ly);
    TEST_ASSERT_EQUAL_INT8(-30, copy.domeSpeed);
    TEST_ASSERT_EQUAL_UINT8(5, copy.audioTrack);
    TEST_ASSERT_EQUAL_UINT8(20, copy.volume);
    TEST_ASSERT_EQUAL_UINT8(1, copy.bubbles);
    TEST_ASSERT_EQUAL_STRING("du", copy.button);
    TEST_ASSERT_EQUAL_INT16(101, copy.macro[0]);
    TEST_ASSERT_EQUAL_INT16(103, copy.macro[1]);
    TEST_ASSERT_EQUAL_INT8(1, copy.connectionStatus);
}

void test_BodyTelemetry_field_copy(void) {
    BodyTelemetry t = {};
    t.msgType = 4;
    t.connected = 1;
    t.motorsActive = 1;
    t.audioPlaying = 0;
    t.uptimeMs = 123456;

    BodyTelemetry copy;
    memcpy(&copy, &t, sizeof(BodyTelemetry));

    TEST_ASSERT_EQUAL_INT8(4, copy.msgType);
    TEST_ASSERT_EQUAL_UINT8(1, copy.connected);
    TEST_ASSERT_EQUAL_UINT8(1, copy.motorsActive);
    TEST_ASSERT_EQUAL_UINT8(0, copy.audioPlaying);
    TEST_ASSERT_EQUAL_UINT32(123456, copy.uptimeMs);
}

// --- Status constants ---
void test_status_constants_have_expected_values(void) {
    TEST_ASSERT_EQUAL(0, STATUS_BUTTONS);
    TEST_ASSERT_EQUAL(1, STATUS_MOVEMENT);
    TEST_ASSERT_EQUAL(2, STATUS_SETTINGS);
    TEST_ASSERT_EQUAL(3, STATUS_AP_CONTROL);
    TEST_ASSERT_EQUAL(-1, STATUS_CONNECTION);
}

void test_BodyCommand_apRequested_field_copy(void) {
    BodyCommand cmd = {};
    cmd.status = STATUS_AP_CONTROL;
    cmd.apRequested = 1;

    BodyCommand copy;
    memcpy(&copy, &cmd, sizeof(BodyCommand));

    TEST_ASSERT_EQUAL_INT8(STATUS_AP_CONTROL, copy.status);
    TEST_ASSERT_EQUAL_UINT8(1, copy.apRequested);
}

void test_BodyCommand_button_max_length(void) {
    BodyCommand cmd = {};
    strncpy(cmd.button, "1234567", sizeof(cmd.button));
    TEST_ASSERT_EQUAL_STRING("1234567", cmd.button);
}

void test_BodyCommand_macro_array_holds_4(void) {
    BodyCommand cmd = {};
    cmd.macro[0] = 101;
    cmd.macro[1] = 103;
    cmd.macro[2] = 107;
    cmd.macro[3] = 200;
    TEST_ASSERT_EQUAL_INT16(101, cmd.macro[0]);
    TEST_ASSERT_EQUAL_INT16(200, cmd.macro[3]);
}

void test_BodyCommand_drive_range(void) {
    BodyCommand cmd = {};
    cmd.lx = -100;
    cmd.ly = 100;
    cmd.domeSpeed = -100;
    TEST_ASSERT_EQUAL_INT8(-100, cmd.lx);
    TEST_ASSERT_EQUAL_INT8(100, cmd.ly);
    TEST_ASSERT_EQUAL_INT8(-100, cmd.domeSpeed);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_BodyCommand_size_is_26_bytes);
    RUN_TEST(test_BodyTelemetry_size_is_8_bytes);
    RUN_TEST(test_BodyCommand_field_copy);
    RUN_TEST(test_BodyTelemetry_field_copy);
    RUN_TEST(test_status_constants_have_expected_values);
    RUN_TEST(test_BodyCommand_apRequested_field_copy);
    RUN_TEST(test_BodyCommand_button_max_length);
    RUN_TEST(test_BodyCommand_macro_array_holds_4);
    RUN_TEST(test_BodyCommand_drive_range);
    UNITY_END();
    return 0;
}
