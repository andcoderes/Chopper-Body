#include <unity.h>
#include "motor/MotorController.h"
#include "dome/DomeController.h"

static MotorController* mc;
static DomeController* dome;

// Core 3.x ledcAttach/ledcWrite key by GPIO pin, not a channel index.
static const int CH_DOME = PIN_DOME_PWM;

static void resetMockHardware() {
    memset(mock_digital_state, 0, sizeof(mock_digital_state));
    memset(mock_ledc_duty, 0, sizeof(mock_ledc_duty));
    memset(mock_ledc_setup, 0, sizeof(mock_ledc_setup));
    memset(mock_ledc_pin, 0, sizeof(mock_ledc_pin));
    memset(mock_isr_table, 0, sizeof(mock_isr_table));
}

void setUp(void) {
    mock_millis_value = 0;
    resetMockHardware();
    mc = new MotorController();
    mc->setup();
    dome = new DomeController();
    dome->setup(*mc);
}

void tearDown(void) {
    delete dome;
    delete mc;
}

// --- setup ---
void test_setup_attaches_interrupts(void) {
    TEST_ASSERT_NOT_NULL(mock_isr_table[PIN_DOME_ENC_A]);
    TEST_ASSERT_NOT_NULL(mock_isr_table[PIN_DOME_HOME]);
}

// --- encoder counting ---
void test_encoder_counting_forward(void) {
    // Channel B LOW → forward → count increments
    mock_digital_state[PIN_DOME_ENC_B] = LOW;
    for (int i = 0; i < 10; i++) {
        mock_trigger_interrupt(PIN_DOME_ENC_A);
    }
    TEST_ASSERT_EQUAL(10, dome->getEncoderCount());
}

void test_encoder_counting_reverse(void) {
    // Channel B HIGH → reverse → count decrements
    mock_digital_state[PIN_DOME_ENC_B] = HIGH;
    for (int i = 0; i < 10; i++) {
        mock_trigger_interrupt(PIN_DOME_ENC_A);
    }
    TEST_ASSERT_EQUAL(-10, dome->getEncoderCount());
}

// --- calibration state machine ---
void test_calibration_idle_by_default(void) {
    TEST_ASSERT_FALSE(dome->isCalibrating());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, dome->getGearRatio());
}

void test_calibration_state_machine(void) {
    // Start calibration
    dome->startCalibration();
    TEST_ASSERT_TRUE(dome->isCalibrating());

    // SEEKING_HOME: loop drives dome, simulate home trigger
    dome->loop();
    TEST_ASSERT_TRUE(mock_ledc_duty[CH_DOME] > 0);  // Motor spinning

    // Trigger home sensor → transitions to ROTATING
    mock_trigger_interrupt(PIN_DOME_HOME);
    dome->loop();

    // Encoder count should have been reset
    TEST_ASSERT_EQUAL(0, dome->getEncoderCount());
    TEST_ASSERT_TRUE(dome->isCalibrating());

    // ROTATING: simulate encoder pulses for one dome revolution
    mock_digital_state[PIN_DOME_ENC_B] = LOW;  // Forward
    for (int i = 0; i < 32000; i++) {
        mock_trigger_interrupt(PIN_DOME_ENC_A);
    }

    // Trigger home sensor again → transitions to DONE
    mock_trigger_interrupt(PIN_DOME_HOME);
    dome->loop();  // Processes ROTATING → DONE

    // DONE: loop stops motor and returns to IDLE
    dome->loop();
    TEST_ASSERT_FALSE(dome->isCalibrating());
    TEST_ASSERT_EQUAL(0, mock_ledc_duty[CH_DOME]);  // Motor stopped
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 10.0f, dome->getGearRatio());
}

void test_calibration_times_out_if_home_sensor_never_fires(void) {
    // Regression: SEEKING_HOME had no elapsed-time bound before this fix —
    // a miswired/failed home sensor would spin the dome motor forever.
    dome->startCalibration();
    dome->loop();
    TEST_ASSERT_TRUE(mock_ledc_duty[CH_DOME] > 0);  // motor spinning

    mock_millis_value = DOME_CALIBRATION_TIMEOUT_MS - 1;
    dome->loop();
    TEST_ASSERT_TRUE(dome->isCalibrating());  // not yet timed out

    mock_millis_value = DOME_CALIBRATION_TIMEOUT_MS;
    dome->loop();
    TEST_ASSERT_FALSE(dome->isCalibrating());
    TEST_ASSERT_EQUAL(0, mock_ledc_duty[CH_DOME]);  // motor stopped
}

void test_calibration_times_out_if_rotation_never_completes(void) {
    // Home found once (enters ROTATING) but the second home trigger
    // (end of one full revolution) never arrives.
    dome->startCalibration();
    dome->loop();
    mock_trigger_interrupt(PIN_DOME_HOME);
    dome->loop();  // SEEKING_HOME -> ROTATING
    TEST_ASSERT_TRUE(dome->isCalibrating());

    mock_millis_value = DOME_CALIBRATION_TIMEOUT_MS;
    dome->loop();
    TEST_ASSERT_FALSE(dome->isCalibrating());
    TEST_ASSERT_EQUAL(0, mock_ledc_duty[CH_DOME]);
}

void test_gear_ratio_calculation(void) {
    // Run a calibration with a known count
    dome->startCalibration();

    // Seek home
    dome->loop();
    mock_trigger_interrupt(PIN_DOME_HOME);
    dome->loop();

    // 16000 ticks → ratio = 16000 / 3200 = 5.0
    mock_digital_state[PIN_DOME_ENC_B] = LOW;
    for (int i = 0; i < 16000; i++) {
        mock_trigger_interrupt(PIN_DOME_ENC_A);
    }

    mock_trigger_interrupt(PIN_DOME_HOME);
    dome->loop();  // ROTATING → DONE
    dome->loop();  // DONE → IDLE

    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, dome->getGearRatio());
}

void test_getRotations_converts_ticks(void) {
    // 6400 ticks / 3200 CPR = 2.0 rotations
    mock_digital_state[PIN_DOME_ENC_B] = LOW;
    for (int i = 0; i < 6400; i++) {
        mock_trigger_interrupt(PIN_DOME_ENC_A);
    }
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, dome->getRotations());
}

void test_getRotations_negative(void) {
    // -3200 ticks / 3200 CPR = -1.0 rotations
    mock_digital_state[PIN_DOME_ENC_B] = HIGH;
    for (int i = 0; i < 3200; i++) {
        mock_trigger_interrupt(PIN_DOME_ENC_A);
    }
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, dome->getRotations());
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_setup_attaches_interrupts);
    RUN_TEST(test_encoder_counting_forward);
    RUN_TEST(test_encoder_counting_reverse);
    RUN_TEST(test_calibration_idle_by_default);
    RUN_TEST(test_calibration_state_machine);
    RUN_TEST(test_calibration_times_out_if_home_sensor_never_fires);
    RUN_TEST(test_calibration_times_out_if_rotation_never_completes);
    RUN_TEST(test_gear_ratio_calculation);
    RUN_TEST(test_getRotations_converts_ticks);
    RUN_TEST(test_getRotations_negative);
    UNITY_END();
    return 0;
}
