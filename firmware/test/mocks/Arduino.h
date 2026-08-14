#pragma once

// Mock Arduino.h for native unit tests

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

// --- Arduino types ---
typedef uint8_t byte;
typedef bool boolean;

// --- Constants ---
#define HIGH 1
#define LOW  0
#define INPUT  0
#define OUTPUT 1
#define INPUT_PULLUP 2

// --- Interrupt constants ---
#define RISING  1
#define FALLING 2
#define CHANGE  3

// --- Attribute stubs ---
#define IRAM_ATTR

#define SERIAL_8N1 0x800001c

#define WIFI_IF_STA 0
#define WIFI_STA    1

// Arduino min/max/constrain macros
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define constrain(x,lo,hi) ((x)<(lo)?(lo):((x)>(hi)?(hi):(x)))

// --- Controllable millis ---
extern unsigned long mock_millis_value;
inline unsigned long millis() { return mock_millis_value; }

// --- Mock GPIO state ---
extern uint8_t mock_digital_state[40];  // pin -> last written value

// --- Mock LEDC state ---
extern uint32_t mock_ledc_duty[40];     // pin -> duty cycle
extern bool     mock_ledc_setup[40];    // pin -> was setup/attach called
extern uint8_t  mock_ledc_pin[40];      // pin -> attached pin (identity, kept for API parity)

// --- Stubs ---
inline void delay(unsigned long) {}
inline long random(long max) { return std::rand() % max; }
inline long random(long min, long max) {
    if (max <= min) return min;
    return min + (std::rand() % (max - min));
}
inline void randomSeed(unsigned long) {}
inline void pinMode(uint8_t, uint8_t) {}
inline void digitalWrite(uint8_t pin, uint8_t value) {
    if (pin < 40) mock_digital_state[pin] = value;
}
inline int  digitalRead(uint8_t pin) { return (pin < 40) ? mock_digital_state[pin] : 0; }
inline int  analogRead(uint8_t)  { return 0; }
inline void noInterrupts() {}
inline void interrupts() {}

// --- Cross-core critical section stubs (single-threaded in native tests) ---
typedef int portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
inline void portENTER_CRITICAL(portMUX_TYPE*) {}
inline void portEXIT_CRITICAL(portMUX_TYPE*) {}

// --- Interrupt mock infrastructure ---
typedef void (*voidFuncPtr)(void);
extern voidFuncPtr mock_isr_table[40];

inline int digitalPinToInterrupt(int pin) { return pin; }
inline void attachInterrupt(uint8_t pin, voidFuncPtr isr, int mode) {
    (void)mode;
    if (pin < 40) mock_isr_table[pin] = isr;
}

// Helper for tests to simulate an interrupt firing on a pin
inline void mock_trigger_interrupt(uint8_t pin) {
    if (pin < 40 && mock_isr_table[pin]) mock_isr_table[pin]();
}

// --- LEDC stubs ---
inline void ledcSetup(uint8_t channel, uint32_t, uint8_t) {
    if (channel < 40) mock_ledc_setup[channel] = true;
}
inline void ledcAttachPin(uint8_t pin, uint8_t channel) {
    if (channel < 40) mock_ledc_pin[channel] = pin;
}
// ESP32 Arduino core 3.x API: attaches directly to a pin (channel is
// allocated internally), keyed here by pin number instead of channel.
inline void ledcAttach(uint8_t pin, uint32_t, uint8_t) {
    if (pin < 40) { mock_ledc_setup[pin] = true; mock_ledc_pin[pin] = pin; }
}
inline void ledcWrite(uint8_t pin, uint32_t duty) {
    if (pin < 40) mock_ledc_duty[pin] = duty;
}

// --- Minimal String class (backed by std::string) ---
class String {
public:
    String() : s_() {}
    String(const char* c) : s_(c ? c : "") {}
    String(const String& o) : s_(o.s_) {}
    String(int val) : s_(std::to_string(val)) {}
    String(long val) : s_(std::to_string(val)) {}
    String(unsigned long val) : s_(std::to_string(val)) {}

    String& operator=(const String& o) { s_ = o.s_; return *this; }
    String& operator=(const char* c)   { s_ = c ? c : ""; return *this; }

    String  operator+(const String& o) const { return String((s_ + o.s_).c_str()); }
    String& operator+=(const String& o) { s_ += o.s_; return *this; }
    String& operator+=(const char* c)   { s_ += c ? c : ""; return *this; }

    bool operator==(const String& o) const { return s_ == o.s_; }
    bool operator==(const char* c)   const { return s_ == (c ? c : ""); }
    bool operator!=(const String& o) const { return s_ != o.s_; }
    bool operator!=(const char* c)   const { return s_ != (c ? c : ""); }

    const char* c_str() const { return s_.c_str(); }
    int length() const { return (int)s_.size(); }

    friend bool operator==(const char* lhs, const String& rhs) {
        return rhs == lhs;
    }

private:
    std::string s_;
};

// --- HardwareSerial stub ---
class HardwareSerial {
public:
    void begin(unsigned long) {}
    void begin(unsigned long, int, int, int) {}
    void print(const char*) {}
    void print(int) {}
    void print(unsigned long) {}
    void println(const char* = "") {}
    void println(int) {}
    void println(unsigned long) {}
    void println(const String& s) { (void)s; }
    int  available() { return mockAvailable; }
    int  read()      { return mockReadValue; }
    void write(uint8_t b) { lastWrittenByte = b; if (writePos < 256) writtenBytes[writePos++] = b; writeCallCount++; }
    size_t write(const uint8_t* buf, size_t len) {
        for (size_t i = 0; i < len && writePos < 256; i++) {
            writtenBytes[writePos++] = buf[i];
        }
        writeCallCount++;
        return len;
    }
    String macAddress() { return String("AA:BB:CC:DD:EE:FF"); }

    void resetMock() {
        mockAvailable = 0;
        mockReadValue = 0;
        lastWrittenByte = 0;
        writeCallCount = 0;
        writePos = 0;
        memset(writtenBytes, 0, sizeof(writtenBytes));
    }

    // --- Test inspection ---
    int mockAvailable = 0;
    int mockReadValue = 0;
    uint8_t lastWrittenByte = 0;
    int writeCallCount = 0;
    uint8_t writtenBytes[256] = {};
    int writePos = 0;
};

// --- Global serial instances (defined in mock_arduino.cpp) ---
extern HardwareSerial Serial;
extern HardwareSerial Serial1;
extern HardwareSerial Serial2;
