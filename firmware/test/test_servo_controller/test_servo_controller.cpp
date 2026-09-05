#include <unity.h>
#include "servo/ServoController.h"

static ServoController* sc;

void setUp(void) {
    sc = new ServoController();
    mock_millis_value = 0;
}

void tearDown(void) {
    delete sc;
}

// --- animate: per-key script cycling ---

void test_animate_l1_cycles_0_1_2(void) {
    const int16_t m[] = {0, 0, 0, 0};
    sc->animate("l1", m, 4);
    TEST_ASSERT_EQUAL(0, sc->maestro_.lastScriptRestarted);
    sc->animate("l1", m, 4);
    TEST_ASSERT_EQUAL(1, sc->maestro_.lastScriptRestarted);
    sc->animate("l1", m, 4);
    TEST_ASSERT_EQUAL(2, sc->maestro_.lastScriptRestarted);
    sc->animate("l1", m, 4);            // wraps
    TEST_ASSERT_EQUAL(0, sc->maestro_.lastScriptRestarted);
}

void test_animate_a_cycles_5_6(void) {
    const int16_t m[] = {0, 0, 0, 0};
    sc->animate("a", m, 4);
    TEST_ASSERT_EQUAL(5, sc->maestro_.lastScriptRestarted);
    sc->animate("a", m, 4);
    TEST_ASSERT_EQUAL(6, sc->maestro_.lastScriptRestarted);
    sc->animate("a", m, 4);            // scriptLen 2 -> wraps back to 5
    TEST_ASSERT_EQUAL(5, sc->maestro_.lastScriptRestarted);
}

void test_animate_stops_script_and_zeroes_ch5_before_running(void) {
    const int16_t m[] = {0, 0, 0, 0};
    sc->animate("l1", m, 4);
    TEST_ASSERT_EQUAL(1, sc->maestro_.stopCallCount);   // stopped first
    TEST_ASSERT_EQUAL(5, sc->maestro_.lastChannel);     // setTarget(5, 0)
    TEST_ASSERT_EQUAL(0, sc->maestro_.lastTarget);
}

void test_animate_r1_step2_is_direct_target_not_script(void) {
    const int16_t m[] = {0, 0, 0, 0};
    sc->animate("r1", m, 4);                            // step 0 -> script 3
    TEST_ASSERT_EQUAL(3, sc->maestro_.lastScriptRestarted);
    TEST_ASSERT_EQUAL(1, sc->maestro_.restartCallCount);

    sc->animate("r1", m, 4);                            // step 1 -> sentinel 100
    TEST_ASSERT_EQUAL(1, sc->maestro_.restartCallCount);  // no new restartScript
    TEST_ASSERT_EQUAL(5, sc->maestro_.lastChannel);       // last setTarget was (5, 2000)
    TEST_ASSERT_EQUAL(2000, sc->maestro_.lastTarget);

    sc->animate("r1", m, 4);                            // step 2 -> script 4
    TEST_ASSERT_EQUAL(4, sc->maestro_.lastScriptRestarted);
}

void test_animate_macro_102_runs_script_7(void) {
    const int16_t m[] = {102, 0, 0, 0};
    sc->animate("", m, 4);
    TEST_ASSERT_EQUAL(7, sc->maestro_.lastScriptRestarted);
}

void test_animate_macro_107_runs_script_8(void) {
    const int16_t m[] = {107, 0, 0, 0};
    sc->animate("", m, 4);
    TEST_ASSERT_EQUAL(8, sc->maestro_.lastScriptRestarted);
}

void test_animate_unknown_key_does_nothing(void) {
    const int16_t m[] = {0, 0, 0, 0};
    sc->animate("zz", m, 4);
    TEST_ASSERT_EQUAL(0, sc->maestro_.restartCallCount);
    TEST_ASSERT_EQUAL(0, sc->maestro_.stopCallCount);
}

void test_animate_no_button_no_macro_does_nothing(void) {
    const int16_t m[] = {0, 0, 0, 0};
    sc->animate("", m, 4);
    TEST_ASSERT_EQUAL(0, sc->maestro_.restartCallCount);
    TEST_ASSERT_EQUAL(0, sc->maestro_.stopCallCount);
}

// --- stop ---
void test_stop_calls_maestro_stop(void) {
    sc->stop();
    TEST_ASSERT_EQUAL(1, sc->maestro_.stopCallCount);
}

// --- checkConnection ---
void test_servo_starts_not_ready(void) {
    TEST_ASSERT_FALSE(sc->isReady());
}

void test_checkConnection_sends_getErrors_command(void) {
    Serial1.resetMock();
    sc->checkConnection();
    TEST_ASSERT_EQUAL(0xA1, Serial1.lastWrittenByte);
}

void test_checkConnection_returns_true_when_response_available(void) {
    Serial1.resetMock();
    sc->checkConnection(); // sends command
    Serial1.mockAvailable = 2;
    Serial1.mockReadValue = 0;
    bool result = sc->checkConnection();
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(sc->isReady());
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_animate_l1_cycles_0_1_2);
    RUN_TEST(test_animate_a_cycles_5_6);
    RUN_TEST(test_animate_stops_script_and_zeroes_ch5_before_running);
    RUN_TEST(test_animate_r1_step2_is_direct_target_not_script);
    RUN_TEST(test_animate_macro_102_runs_script_7);
    RUN_TEST(test_animate_macro_107_runs_script_8);
    RUN_TEST(test_animate_unknown_key_does_nothing);
    RUN_TEST(test_animate_no_button_no_macro_does_nothing);
    RUN_TEST(test_stop_calls_maestro_stop);
    RUN_TEST(test_servo_starts_not_ready);
    RUN_TEST(test_checkConnection_sends_getErrors_command);
    RUN_TEST(test_checkConnection_returns_true_when_response_available);
    UNITY_END();
    return 0;
}
