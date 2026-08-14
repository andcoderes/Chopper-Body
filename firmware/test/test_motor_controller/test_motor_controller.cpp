#include <unity.h>
#include "motor/MotorController.h"

static MotorController* mc;

// Core 3.x ledcAttach/ledcWrite key by GPIO pin, not a channel index.
static const int CH_LEFT = PIN_MDDS_AN1;
static const int CH_RIGHT = PIN_MDDS_AN2;
static const int CH_DOME = PIN_DOME_PWM;

// Reset all mock state
static void resetMockHardware() {
    memset(mock_digital_state, 0, sizeof(mock_digital_state));
    memset(mock_ledc_duty, 0, sizeof(mock_ledc_duty));
    memset(mock_ledc_setup, 0, sizeof(mock_ledc_setup));
    memset(mock_ledc_pin, 0, sizeof(mock_ledc_pin));
}

void setUp(void) {
    mock_millis_value = 0;
    resetMockHardware();
    mc = new MotorController();
    mc->setup();
}

void tearDown(void) {
    delete mc;
}

// --- setup ---
void test_setup_configures_ledc_channels(void) {
    TEST_ASSERT_TRUE(mock_ledc_setup[CH_LEFT]);
    TEST_ASSERT_TRUE(mock_ledc_setup[CH_RIGHT]);
    TEST_ASSERT_TRUE(mock_ledc_setup[CH_DOME]);
    TEST_ASSERT_EQUAL(PIN_MDDS_AN1, mock_ledc_pin[CH_LEFT]);
    TEST_ASSERT_EQUAL(PIN_MDDS_AN2, mock_ledc_pin[CH_RIGHT]);
    TEST_ASSERT_EQUAL(PIN_DOME_PWM, mock_ledc_pin[CH_DOME]);
}

// --- stopAll ---
void test_stopAll_zeros_all_motors(void) {
    mc->setDrive(50, 50);
    mc->stopAll();
    TEST_ASSERT_EQUAL(0, mock_ledc_duty[CH_LEFT]);
    TEST_ASSERT_EQUAL(0, mock_ledc_duty[CH_RIGHT]);
    TEST_ASSERT_EQUAL(0, mock_ledc_duty[CH_DOME]);
    TEST_ASSERT_EQUAL(LOW, mock_digital_state[PIN_DOME_DIR]);
    TEST_ASSERT_FALSE(mc->isActive());
}

// --- setDrive: differential mixing ---
void test_setDrive_forward(void) {
    mc->setDrive(0, 100);
    // left = 100+0 = 100, right = 100-0 = 100
    // duty = 100 * 2.55 = 255
    TEST_ASSERT_EQUAL(255, mock_ledc_duty[CH_LEFT]);
    TEST_ASSERT_EQUAL(255, mock_ledc_duty[CH_RIGHT]);
    // direction: forward = LOW
    TEST_ASSERT_EQUAL(LOW, mock_digital_state[PIN_MDDS_IN1]);
    TEST_ASSERT_EQUAL(LOW, mock_digital_state[PIN_MDDS_IN2]);
    TEST_ASSERT_TRUE(mc->isActive());
}

void test_setDrive_reverse(void) {
    mc->setDrive(0, -100);
    // left = -100, right = -100
    TEST_ASSERT_EQUAL(255, mock_ledc_duty[CH_LEFT]);
    TEST_ASSERT_EQUAL(255, mock_ledc_duty[CH_RIGHT]);
    // direction: reverse = HIGH
    TEST_ASSERT_EQUAL(HIGH, mock_digital_state[PIN_MDDS_IN1]);
    TEST_ASSERT_EQUAL(HIGH, mock_digital_state[PIN_MDDS_IN2]);
}

void test_setDrive_turn_right(void) {
    mc->setDrive(100, 0);
    // left = 0+100 = 100, right = 0-100 = -100
    TEST_ASSERT_EQUAL(255, mock_ledc_duty[CH_LEFT]);
    TEST_ASSERT_EQUAL(255, mock_ledc_duty[CH_RIGHT]);
    TEST_ASSERT_EQUAL(LOW, mock_digital_state[PIN_MDDS_IN1]);   // left fwd
    TEST_ASSERT_EQUAL(HIGH, mock_digital_state[PIN_MDDS_IN2]);  // right rev
}

void test_setDrive_turn_left(void) {
    mc->setDrive(-100, 0);
    // left = 0+(-100) = -100, right = 0-(-100) = 100
    TEST_ASSERT_EQUAL(255, mock_ledc_duty[CH_LEFT]);
    TEST_ASSERT_EQUAL(255, mock_ledc_duty[CH_RIGHT]);
    TEST_ASSERT_EQUAL(HIGH, mock_digital_state[PIN_MDDS_IN1]);  // left rev
    TEST_ASSERT_EQUAL(LOW, mock_digital_state[PIN_MDDS_IN2]);   // right fwd
}

void test_setDrive_mixed(void) {
    mc->setDrive(50, 50);
    // left = 50+50 = 100, right = 50-50 = 0
    TEST_ASSERT_EQUAL(255, mock_ledc_duty[CH_LEFT]);
    TEST_ASSERT_EQUAL(0, mock_ledc_duty[CH_RIGHT]);
}

void test_setDrive_clamps_values(void) {
    mc->setDrive(100, 100);
    // left = 100+100 = 200 -> clamped to 100
    // right = 100-100 = 0
    TEST_ASSERT_EQUAL(255, mock_ledc_duty[CH_LEFT]);
    TEST_ASSERT_EQUAL(0, mock_ledc_duty[CH_RIGHT]);
}

void test_setDrive_zero_is_inactive(void) {
    mc->setDrive(0, 0);
    TEST_ASSERT_FALSE(mc->isActive());
    TEST_ASSERT_EQUAL(0, mock_ledc_duty[CH_LEFT]);
    TEST_ASSERT_EQUAL(0, mock_ledc_duty[CH_RIGHT]);
}

// --- setDome ---
void test_setDome_positive(void) {
    mc->setDome(100);
    TEST_ASSERT_EQUAL(LOW, mock_digital_state[PIN_DOME_DIR]);
    TEST_ASSERT_EQUAL(255, mock_ledc_duty[CH_DOME]);
}

void test_setDome_negative(void) {
    mc->setDome(-100);
    TEST_ASSERT_EQUAL(HIGH, mock_digital_state[PIN_DOME_DIR]);
    TEST_ASSERT_EQUAL(255, mock_ledc_duty[CH_DOME]);
}

void test_setDome_zero(void) {
    mc->setDome(50);
    mc->setDome(0);
    TEST_ASSERT_EQUAL(LOW, mock_digital_state[PIN_DOME_DIR]);
    TEST_ASSERT_EQUAL(0, mock_ledc_duty[CH_DOME]);
}

void test_setDome_half_speed(void) {
    mc->setDome(50);
    // duty = 50 * 2.55 = 127
    TEST_ASSERT_EQUAL(127, mock_ledc_duty[CH_DOME]);
}

void test_setDome_nonzero_is_active(void) {
    mc->setDome(50);
    TEST_ASSERT_TRUE(mc->isActive());
}

void test_setDome_zero_is_inactive(void) {
    mc->setDome(0);
    TEST_ASSERT_FALSE(mc->isActive());
}

// --- checkTimeout ---
void test_checkTimeout_stops_after_timeout(void) {
    mock_millis_value = 1000;
    mc->setDrive(0, 100);
    TEST_ASSERT_TRUE(mc->isActive());

    mock_millis_value = 1000 + MOTOR_TIMEOUT_MS;
    mc->checkTimeout();
    TEST_ASSERT_FALSE(mc->isActive());
}

void test_checkTimeout_does_not_stop_before_timeout(void) {
    mock_millis_value = 1000;
    mc->setDrive(0, 100);

    mock_millis_value = 1000 + MOTOR_TIMEOUT_MS - 1;
    mc->checkTimeout();
    TEST_ASSERT_TRUE(mc->isActive());
}

void test_checkTimeout_noop_when_inactive(void) {
    mc->stopAll();
    mock_millis_value = 99999;
    mc->checkTimeout();  // should not crash or change state
    TEST_ASSERT_FALSE(mc->isActive());
}

// Regression: setDome() alone previously never set anything "active", so
// a dome-only command bypassed this timeout entirely (only the drive
// wheels were covered).
void test_checkTimeout_stops_dome_only_motion(void) {
    mock_millis_value = 1000;
    mc->setDome(50);
    TEST_ASSERT_TRUE(mc->isActive());

    mock_millis_value = 1000 + MOTOR_TIMEOUT_MS;
    mc->checkTimeout();
    TEST_ASSERT_FALSE(mc->isActive());
    TEST_ASSERT_EQUAL(0, mock_ledc_duty[CH_DOME]);
}

void test_checkTimeout_preserves_dome_when_only_drive_cleared(void) {
    // Drive going idle must not silently clear dome's active state —
    // isActive() should stay true (OR of both), not overwritten by an
    // unrelated setDrive(0,0) call.
    mock_millis_value = 1000;
    mc->setDome(50);
    mc->setDrive(0, 0);
    TEST_ASSERT_TRUE(mc->isActive());

    mock_millis_value = 1000 + MOTOR_TIMEOUT_MS - 1;
    mc->checkTimeout();
    TEST_ASSERT_TRUE(mc->isActive());
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_setup_configures_ledc_channels);
    RUN_TEST(test_stopAll_zeros_all_motors);
    RUN_TEST(test_setDrive_forward);
    RUN_TEST(test_setDrive_reverse);
    RUN_TEST(test_setDrive_turn_right);
    RUN_TEST(test_setDrive_turn_left);
    RUN_TEST(test_setDrive_mixed);
    RUN_TEST(test_setDrive_clamps_values);
    RUN_TEST(test_setDrive_zero_is_inactive);
    RUN_TEST(test_setDome_positive);
    RUN_TEST(test_setDome_negative);
    RUN_TEST(test_setDome_zero);
    RUN_TEST(test_setDome_half_speed);
    RUN_TEST(test_setDome_nonzero_is_active);
    RUN_TEST(test_setDome_zero_is_inactive);
    RUN_TEST(test_checkTimeout_stops_after_timeout);
    RUN_TEST(test_checkTimeout_does_not_stop_before_timeout);
    RUN_TEST(test_checkTimeout_noop_when_inactive);
    RUN_TEST(test_checkTimeout_stops_dome_only_motion);
    RUN_TEST(test_checkTimeout_preserves_dome_when_only_drive_cleared);
    UNITY_END();
    return 0;
}
