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

// --- checkAnimation state machine ---

void test_checkAnimation_arm_out_always_allowed(void) {
    TEST_ASSERT_TRUE(sc->checkAnimation(0));
    TEST_ASSERT_TRUE(sc->handOut_);
}

void test_checkAnimation_arm_in_blocked_when_not_out(void) {
    TEST_ASSERT_FALSE(sc->checkAnimation(1));
}

void test_checkAnimation_arm_in_allowed_when_out(void) {
    sc->checkAnimation(0); // arm out
    TEST_ASSERT_TRUE(sc->checkAnimation(1)); // arm in
    TEST_ASSERT_FALSE(sc->handOut_);
}

void test_checkAnimation_say_hi_requires_arm_out(void) {
    TEST_ASSERT_FALSE(sc->checkAnimation(2)); // blocked
    sc->checkAnimation(0); // arm out
    TEST_ASSERT_TRUE(sc->checkAnimation(2)); // now allowed
}

void test_checkAnimation_say_hi_left_requires_arm_out(void) {
    TEST_ASSERT_FALSE(sc->checkAnimation(3)); // blocked
    sc->checkAnimation(0); // arm out
    TEST_ASSERT_TRUE(sc->checkAnimation(3)); // now allowed
}

void test_checkAnimation_periscope_scripts_always_allowed(void) {
    TEST_ASSERT_TRUE(sc->checkAnimation(4));
    TEST_ASSERT_TRUE(sc->checkAnimation(5));
    TEST_ASSERT_TRUE(sc->checkAnimation(6));
}

// --- animate: button routing ---

void test_animate_button_y_first_press_runs_arm_out(void) {
    const int16_t macros[] = {0, 0, 0, 0};
    sc->animate("y", macros, 4);
    TEST_ASSERT_EQUAL(0, sc->maestro_.lastScriptRestarted);
    TEST_ASSERT_EQUAL(1, sc->maestro_.restartCallCount);
}

void test_animate_button_y_toggle_runs_arm_in(void) {
    const int16_t macros[] = {0, 0, 0, 0};
    sc->animate("y", macros, 4); // arm out (script 0)
    sc->animate("y", macros, 4); // arm in  (script 1)
    TEST_ASSERT_EQUAL(1, sc->maestro_.lastScriptRestarted);
    TEST_ASSERT_EQUAL(2, sc->maestro_.restartCallCount);
}

void test_animate_button_du_runs_script_4(void) {
    const int16_t macros[] = {0, 0, 0, 0};
    sc->animate("du", macros, 4);
    TEST_ASSERT_EQUAL(4, sc->maestro_.lastScriptRestarted);
}

void test_animate_button_dd_runs_script_5(void) {
    const int16_t macros[] = {0, 0, 0, 0};
    sc->animate("dd", macros, 4);
    TEST_ASSERT_EQUAL(5, sc->maestro_.lastScriptRestarted);
}

void test_animate_unknown_button_does_nothing(void) {
    const int16_t macros[] = {0, 0, 0, 0};
    sc->animate("zz", macros, 4);
    TEST_ASSERT_EQUAL(0, sc->maestro_.restartCallCount);
}

// --- animate: empty button + macros ---
void test_animate_empty_button_with_macros(void) {
    const int16_t macros[] = {101, 0, 0, 0};
    sc->animate("", macros, 4);
    // "101" doesn't match any key, so no restart
    TEST_ASSERT_EQUAL(0, sc->maestro_.restartCallCount);
}

// --- execute ---
void test_execute_runs_independent_script(void) {
    sc->execute(5);
    TEST_ASSERT_EQUAL(5, sc->maestro_.lastScriptRestarted);
    TEST_ASSERT_EQUAL(1, sc->maestro_.restartCallCount);
}

void test_execute_blocks_arm_dependent_when_not_out(void) {
    sc->execute(2); // requires arm out
    TEST_ASSERT_EQUAL(0, sc->maestro_.restartCallCount);
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
    RUN_TEST(test_checkAnimation_arm_out_always_allowed);
    RUN_TEST(test_checkAnimation_arm_in_blocked_when_not_out);
    RUN_TEST(test_checkAnimation_arm_in_allowed_when_out);
    RUN_TEST(test_checkAnimation_say_hi_requires_arm_out);
    RUN_TEST(test_checkAnimation_say_hi_left_requires_arm_out);
    RUN_TEST(test_checkAnimation_periscope_scripts_always_allowed);
    RUN_TEST(test_animate_button_y_first_press_runs_arm_out);
    RUN_TEST(test_animate_button_y_toggle_runs_arm_in);
    RUN_TEST(test_animate_button_du_runs_script_4);
    RUN_TEST(test_animate_button_dd_runs_script_5);
    RUN_TEST(test_animate_unknown_button_does_nothing);
    RUN_TEST(test_animate_empty_button_with_macros);
    RUN_TEST(test_execute_runs_independent_script);
    RUN_TEST(test_execute_blocks_arm_dependent_when_not_out);
    RUN_TEST(test_stop_calls_maestro_stop);
    RUN_TEST(test_servo_starts_not_ready);
    RUN_TEST(test_checkConnection_sends_getErrors_command);
    RUN_TEST(test_checkConnection_returns_true_when_response_available);
    UNITY_END();
    return 0;
}
