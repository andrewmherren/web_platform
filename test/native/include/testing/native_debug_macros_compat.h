#ifndef NATIVE_DEBUG_MACROS_COMPAT_H
#define NATIVE_DEBUG_MACROS_COMPAT_H

// ArduinoFake's Serial mock has no printf() (an ESP32-only Print
// extension), but interface/debug_macros.h's WARN_PRINTF/ERROR_PRINTF are
// unconditionally enabled (not gated behind DEBUG_ENABLED) and call
// Serial.printf() directly, so they fail to compile natively as soon as a
// file that uses them is included in the native build.
//
// No test asserts on their output, so route them to nothing rather than
// trying to reproduce real printf-style output through ArduinoFake's
// Serial mock: routing through Serial.print() here was tried first and
// reliably segfaulted a few driver-construct/destroy cycles later (heap
// corruption inside ArduinoFake's Serial mock, not this file) - not worth
// chasing for output nothing checks. This matches how DEBUG_PRINTF already
// behaves when WEB_PLATFORM_DEBUG is unset (the default): compiles to
// nothing.
//
// Include *after* utilities/debug_macros.h, guarded by NATIVE_PLATFORM.

#ifdef NATIVE_PLATFORM

#undef WARN_PRINTF
#define WARN_PRINTF(fmt, ...)

#undef ERROR_PRINTF
#define ERROR_PRINTF(fmt, ...)

#endif // NATIVE_PLATFORM

#endif // NATIVE_DEBUG_MACROS_COMPAT_H
