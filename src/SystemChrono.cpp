/**
 * @file SystemChrono.cpp
 * @brief Implementation of SystemChrono 64-bit time helpers.
 */

#include "SystemChrono/SystemChrono.h"

#include <stdio.h>

#if !defined(ARDUINO)
  #error "SystemChrono: this library currently supports Arduino builds only."
#endif

#if defined(ARDUINO_ARCH_ESP32)
  #include "esp_timer.h"
#endif

namespace SystemChrono {

// ===========================================================================
// Internal: Platform microsecond source
// ===========================================================================

static inline int64_t micros64Impl() {
#if defined(ARDUINO_ARCH_ESP32)
  // ESP32: monotonic microseconds since boot
  return static_cast<int64_t>(esp_timer_get_time());

#elif defined(ARDUINO)
  // Generic Arduino: extend (typically) 32-bit micros() to 64-bit via wrap tracking.
  // Note: If micros() is already 64-bit on a given core, this still works.
  static uint32_t last = 0;
  static uint64_t high = 0;

  noInterrupts();
  uint32_t now = static_cast<uint32_t>(micros());
  if (now < last) {
    high += (1ULL << 32);  // wrapped
  }
  last = now;
  uint64_t full = high | static_cast<uint64_t>(now);
  interrupts();

  return static_cast<int64_t>(full);

#else
  #error "SystemChrono: unsupported platform."
#endif
}

// ===========================================================================
// Public Time Accessors
// ===========================================================================

int64_t micros64() {
  return micros64Impl();
}

int64_t millis64() {
  return micros64Impl() / 1000LL;
}

int64_t seconds64() {
  return micros64Impl() / 1000000LL;
}

int64_t microsSince(int64_t startUs) {
  return micros64Impl() - startUs;
}

int64_t millisSince(int64_t startMs) {
  return millis64() - startMs;
}

int64_t secondsSince(int64_t startS) {
  return seconds64() - startS;
}

String formatTime(int64_t microsSinceBoot) {
  int64_t totalMs = microsSinceBoot / 1000LL;
  bool negative = totalMs < 0;
  if (negative) {
    totalMs = -totalMs;
  }

  int64_t hours = totalMs / 3600000LL;
  int64_t minutes = (totalMs / 60000LL) % 60LL;
  int64_t seconds = (totalMs / 1000LL) % 60LL;
  int64_t millis = totalMs % 1000LL;

  char buf[32];
  snprintf(buf, sizeof(buf), "%s%lld:%02lld:%02lld.%03lld",
           negative ? "-" : "",
           static_cast<long long>(hours),
           static_cast<long long>(minutes),
           static_cast<long long>(seconds),
           static_cast<long long>(millis));
  return String(buf);
}

String formatNow() {
  return formatTime(micros64());
}

// ===========================================================================
// Stopwatch Implementation
// ===========================================================================

Stopwatch::Stopwatch() : _startUs(0), _totalUs(0), _running(false) {}

void Stopwatch::start() {
  _totalUs = 0;
  _startUs = micros64();
  _running = true;
}

void Stopwatch::stop() {
  if (_running) {
    _totalUs += microsSince(_startUs);
    _running = false;
    _startUs = 0;
  }
}

void Stopwatch::resume() {
  if (!_running) {
    _startUs = micros64();
    _running = true;
  }
}

void Stopwatch::reset() {
  _totalUs = 0;
  if (_running) {
    _startUs = micros64();
  } else {
    _startUs = 0;
  }
}

int64_t Stopwatch::elapsedMicros() const {
  int64_t acc = _totalUs;
  if (_running) {
    acc += microsSince(_startUs);
  }
  return acc;
}

int64_t Stopwatch::elapsedMillis() const {
  return elapsedMicros() / 1000LL;
}

int64_t Stopwatch::elapsedSeconds() const {
  return elapsedMicros() / 1000000LL;
}

bool Stopwatch::isRunning() const {
  return _running;
}

// ===========================================================================
// ElapsedMicros64 Implementation
// ===========================================================================

ElapsedMicros64::ElapsedMicros64() {
  _us = micros64Impl();
}

ElapsedMicros64::ElapsedMicros64(int64_t valUs) {
  _us = micros64Impl() - valUs;
}

ElapsedMicros64::ElapsedMicros64(const ElapsedMicros64& orig) {
  _us = orig._us;
}

ElapsedMicros64::operator int64_t() const {
  return micros64Impl() - _us;
}

ElapsedMicros64& ElapsedMicros64::operator=(const ElapsedMicros64& rhs) {
  _us = rhs._us;
  return *this;
}

ElapsedMicros64& ElapsedMicros64::operator=(int64_t valUs) {
  _us = micros64Impl() - valUs;
  return *this;
}

ElapsedMicros64& ElapsedMicros64::operator-=(int64_t valUs) {
  _us += valUs;
  return *this;
}

ElapsedMicros64& ElapsedMicros64::operator+=(int64_t valUs) {
  _us -= valUs;
  return *this;
}

ElapsedMicros64 ElapsedMicros64::operator-(int64_t valUs) const {
  ElapsedMicros64 r(*this);
  r._us += valUs;
  return r;
}

ElapsedMicros64 ElapsedMicros64::operator+(int64_t valUs) const {
  ElapsedMicros64 r(*this);
  r._us -= valUs;
  return r;
}

// ===========================================================================
// ElapsedMillis64 Implementation
// ===========================================================================

ElapsedMillis64::ElapsedMillis64() {
  _us = micros64Impl();
}

ElapsedMillis64::ElapsedMillis64(int64_t valMs) {
  _us = micros64Impl() - valMs * 1000LL;
}

ElapsedMillis64::ElapsedMillis64(const ElapsedMillis64& orig) {
  _us = orig._us;
}

ElapsedMillis64::operator int64_t() const {
  return (micros64Impl() - _us) / 1000LL;
}

ElapsedMillis64& ElapsedMillis64::operator=(const ElapsedMillis64& rhs) {
  _us = rhs._us;
  return *this;
}

ElapsedMillis64& ElapsedMillis64::operator=(int64_t valMs) {
  _us = micros64Impl() - valMs * 1000LL;
  return *this;
}

ElapsedMillis64& ElapsedMillis64::operator-=(int64_t valMs) {
  _us += valMs * 1000LL;
  return *this;
}

ElapsedMillis64& ElapsedMillis64::operator+=(int64_t valMs) {
  _us -= valMs * 1000LL;
  return *this;
}

ElapsedMillis64 ElapsedMillis64::operator-(int64_t valMs) const {
  ElapsedMillis64 r(*this);
  r._us += valMs * 1000LL;
  return r;
}

ElapsedMillis64 ElapsedMillis64::operator+(int64_t valMs) const {
  ElapsedMillis64 r(*this);
  r._us -= valMs * 1000LL;
  return r;
}

// ===========================================================================
// ElapsedSeconds64 Implementation
// ===========================================================================

ElapsedSeconds64::ElapsedSeconds64() {
  _us = micros64Impl();
}

ElapsedSeconds64::ElapsedSeconds64(int64_t valS) {
  _us = micros64Impl() - valS * 1000000LL;
}

ElapsedSeconds64::ElapsedSeconds64(const ElapsedSeconds64& orig) {
  _us = orig._us;
}

ElapsedSeconds64::operator int64_t() const {
  return (micros64Impl() - _us) / 1000000LL;
}

ElapsedSeconds64& ElapsedSeconds64::operator=(const ElapsedSeconds64& rhs) {
  _us = rhs._us;
  return *this;
}

ElapsedSeconds64& ElapsedSeconds64::operator=(int64_t valS) {
  _us = micros64Impl() - valS * 1000000LL;
  return *this;
}

ElapsedSeconds64& ElapsedSeconds64::operator-=(int64_t valS) {
  _us += valS * 1000000LL;
  return *this;
}

ElapsedSeconds64& ElapsedSeconds64::operator+=(int64_t valS) {
  _us -= valS * 1000000LL;
  return *this;
}

ElapsedSeconds64 ElapsedSeconds64::operator-(int64_t valS) const {
  ElapsedSeconds64 r(*this);
  r._us += valS * 1000000LL;
  return r;
}

ElapsedSeconds64 ElapsedSeconds64::operator+(int64_t valS) const {
  ElapsedSeconds64 r(*this);
  r._us -= valS * 1000000LL;
  return r;
}

}  // namespace SystemChrono
