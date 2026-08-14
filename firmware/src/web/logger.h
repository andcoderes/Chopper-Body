#pragma once

#include <Arduino.h>

#ifdef UNIT_TEST

inline void logAll(const char* fmt, ...) { (void)fmt; }

#else

inline void logAll(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
inline void logAll(const char* fmt, ...) {
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    Serial.println(buf);
}

#endif
