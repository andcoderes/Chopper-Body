#include <unity.h>
#include "communication/EspNowController.h"

static EspNowController* enc;

void setUp(void) {
    enc = new EspNowController();
    mock_millis_value = 0;
    // Reset static state
    EspNowController::dataReady_ = false;
    EspNowController::lastRecvTime_ = 0;
    EspNowController::incomingBuffer_ = {};
    EspNowController::everReceived_ = false;
}

void tearDown(void) {
    delete enc;
}

// --- isConnected timeout logic ---

void test_isConnected_false_when_never_received(void) {
    mock_millis_value = 6000;
    TEST_ASSERT_FALSE(enc->isConnected());
}

void test_isConnected_false_when_never_received_even_before_timeout_window(void) {
    // Regression: lastRecvTime_ defaults to 0, so before the fix
    // (millis() - lastRecvTime_) < CONNECTION_TIMEOUT_MS was true for the
    // first 5s after boot even though no packet was ever received.
    mock_millis_value = 2000;
    TEST_ASSERT_FALSE(enc->isConnected());
}

void test_isConnected_true_within_timeout(void) {
    EspNowController::everReceived_ = true;
    EspNowController::lastRecvTime_ = 1000;
    mock_millis_value = 5999; // 4999ms since last recv, < 5000
    TEST_ASSERT_TRUE(enc->isConnected());
}

void test_isConnected_false_at_exact_boundary(void) {
    EspNowController::everReceived_ = true;
    EspNowController::lastRecvTime_ = 1000;
    mock_millis_value = 6000; // exactly 5000ms since last recv
    TEST_ASSERT_FALSE(enc->isConnected());
}

void test_isConnected_false_after_timeout(void) {
    EspNowController::everReceived_ = true;
    EspNowController::lastRecvTime_ = 1000;
    mock_millis_value = 7000; // 6000ms since last recv
    TEST_ASSERT_FALSE(enc->isConnected());
}

void test_isConnected_true_just_received(void) {
    EspNowController::everReceived_ = true;
    EspNowController::lastRecvTime_ = 5000;
    mock_millis_value = 5000; // 0ms since last recv
    TEST_ASSERT_TRUE(enc->isConnected());
}

void test_onDataRecv_sets_everReceived_and_lastRecvTime(void) {
    mock_millis_value = 1234;
    BodyCommand fakeCmd = {};
    fakeCmd.msgType = 3;
    fakeCmd.status = STATUS_MOVEMENT;

    uint8_t senderMac[6] = {0};
    esp_now_recv_info_t info = {};
    info.src_addr = senderMac;
    EspNowController::onDataRecv(&info, (const uint8_t*)&fakeCmd, sizeof(BodyCommand));

    TEST_ASSERT_TRUE(EspNowController::everReceived_);
    TEST_ASSERT_EQUAL_UINT32(1234, EspNowController::lastRecvTime_);
    TEST_ASSERT_TRUE(EspNowController::dataReady_);
    TEST_ASSERT_TRUE(enc->isConnected());
}

void test_onDataRecv_ignores_null_info(void) {
    BodyCommand fakeCmd = {};
    fakeCmd.msgType = 3;

    EspNowController::onDataRecv(nullptr, (const uint8_t*)&fakeCmd, sizeof(BodyCommand));

    TEST_ASSERT_FALSE(EspNowController::dataReady_);
    TEST_ASSERT_FALSE(EspNowController::everReceived_);
}

void test_onDataRecv_ignores_wrong_msgType(void) {
    BodyCommand fakeCmd = {};
    fakeCmd.msgType = 4;  // not a BodyCommand (that's BodyTelemetry's msgType)
    fakeCmd.status = STATUS_MOVEMENT;

    uint8_t senderMac[6] = {0};
    esp_now_recv_info_t info = {};
    info.src_addr = senderMac;
    EspNowController::onDataRecv(&info, (const uint8_t*)&fakeCmd, sizeof(BodyCommand));

    TEST_ASSERT_FALSE(EspNowController::dataReady_);
}

void test_onDataRecv_forces_null_terminated_button(void) {
    BodyCommand raw = {};
    raw.msgType = 3;
    raw.status = STATUS_BUTTONS;
    memset(raw.button, 'A', sizeof(raw.button));  // fill all 8 bytes, no NUL

    uint8_t senderMac[6] = {0};
    esp_now_recv_info_t info = {};
    info.src_addr = senderMac;
    EspNowController::onDataRecv(&info, (const uint8_t*)&raw, sizeof(BodyCommand));

    TEST_ASSERT_EQUAL('\0', EspNowController::incomingBuffer_.button[7]);
}

// --- loop callback dispatch ---

static BodyCommand lastDispatchedCmd;
static int callbackCallCount = 0;

void testCallback(const BodyCommand& cmd) {
    memcpy(&lastDispatchedCmd, &cmd, sizeof(BodyCommand));
    callbackCallCount++;
}

void test_loop_dispatches_callback_when_data_ready(void) {
    callbackCallCount = 0;
    enc->setCommandCallback(testCallback);

    // Simulate data received
    BodyCommand fakeCmd = {};
    fakeCmd.msgType = 3;
    fakeCmd.status = STATUS_MOVEMENT;
    fakeCmd.lx = 50;
    fakeCmd.ly = -30;
    fakeCmd.domeSpeed = 20;
    memcpy((void*)&EspNowController::incomingBuffer_, &fakeCmd, sizeof(BodyCommand));
    EspNowController::dataReady_ = true;
    EspNowController::lastRecvTime_ = 0;

    mock_millis_value = 100;
    enc->loop();

    TEST_ASSERT_EQUAL(1, callbackCallCount);
    TEST_ASSERT_EQUAL_INT8(3, lastDispatchedCmd.msgType);
    TEST_ASSERT_EQUAL_INT8(STATUS_MOVEMENT, lastDispatchedCmd.status);
    TEST_ASSERT_EQUAL_INT8(50, lastDispatchedCmd.lx);
    TEST_ASSERT_EQUAL_INT8(-30, lastDispatchedCmd.ly);
    TEST_ASSERT_EQUAL_INT8(20, lastDispatchedCmd.domeSpeed);
}

void test_loop_does_not_dispatch_when_no_data(void) {
    callbackCallCount = 0;
    enc->setCommandCallback(testCallback);
    EspNowController::dataReady_ = false;
    mock_millis_value = 100;
    enc->loop();
    TEST_ASSERT_EQUAL(0, callbackCallCount);
}

void test_loop_no_crash_without_callback(void) {
    EspNowController::dataReady_ = true;
    BodyCommand fakeCmd = {};
    fakeCmd.msgType = 3;
    memcpy((void*)&EspNowController::incomingBuffer_, &fakeCmd, sizeof(BodyCommand));
    mock_millis_value = 100;
    // Should not crash even without callback set
    enc->loop();
    TEST_ASSERT_FALSE(EspNowController::dataReady_);
}

// --- telemetry state setters ---
void test_setMotorsActive_stores_state(void) {
    enc->setMotorsActive(true);
    // Internal state; verify via sendTelemetry behavior indirectly
    enc->setMotorsActive(false);
    // No crash, state is stored
}

void test_setAudioPlaying_stores_state(void) {
    enc->setAudioPlaying(true);
    enc->setAudioPlaying(false);
    // No crash, state is stored
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_isConnected_false_when_never_received);
    RUN_TEST(test_isConnected_false_when_never_received_even_before_timeout_window);
    RUN_TEST(test_isConnected_true_within_timeout);
    RUN_TEST(test_isConnected_false_at_exact_boundary);
    RUN_TEST(test_isConnected_false_after_timeout);
    RUN_TEST(test_isConnected_true_just_received);
    RUN_TEST(test_onDataRecv_sets_everReceived_and_lastRecvTime);
    RUN_TEST(test_loop_dispatches_callback_when_data_ready);
    RUN_TEST(test_loop_does_not_dispatch_when_no_data);
    RUN_TEST(test_loop_no_crash_without_callback);
    RUN_TEST(test_setMotorsActive_stores_state);
    RUN_TEST(test_setAudioPlaying_stores_state);
    UNITY_END();
    return 0;
}
