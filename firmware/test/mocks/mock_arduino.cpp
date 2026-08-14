#include "Arduino.h"
#include "WiFi.h"

// Global mock state
unsigned long mock_millis_value = 0;
uint8_t mock_digital_state[40] = {};
// Sized to 40 (not 16): core 3.x ledcAttach/ledcWrite key by GPIO pin
// number, not a small channel index, and pins here go up to PIN_DOME_PWM=25.
uint32_t mock_ledc_duty[40] = {};
bool mock_ledc_setup[40] = {};
uint8_t mock_ledc_pin[40] = {};
voidFuncPtr mock_isr_table[40] = {};

// Global instances
HardwareSerial Serial;
HardwareSerial Serial1;
HardwareSerial Serial2;
WiFiClass WiFi;
