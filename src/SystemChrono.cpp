/**
 * @file SystemChrono.cpp
 * @brief Implementation of SystemChrono 64-bit time helpers.
 */

#include "SystemChrono/SystemChrono.h"

#include <limits>
#include <stdio.h>

#if !defined(ARDUINO)
  #error "SystemChrono: this library currently supports Arduino builds only."
#endif

#if defined(ARDUINO_ARCH_ESP32)
  #include "esp_timer.h"
#endif

namespace SystemChrono {

namespace {

static constexpr int64_t INT64_MIN_VALUE = (std::numeric_limits<int64_t>::min)();
static constexpr int64_t INT64_MAX_VALUE = (std::numeric_limits<int64_t>::max)();

static inline int64_t saturatingAdd(int64_t lhs, int64_t rhs) {
#if defined(__GNUC__) || defined(__clang__)
  int64_t out = 0;
  if (!__builtin_add_overflow(lhs, rhs, &out)) {
    return out;
  }
  return rhs >= 0 ? INT64_MAX_VALUE : INT64_MIN_VALUE;
#else
  if ((rhs > 0) && (lhs > (INT64_MAX_VALUE - rhs))) {
    return INT64_MAX_VALUE;
  }
  if ((rhs < 0) && (lhs < (INT64_MIN_VALUE - rhs))) {
    return INT64_MIN_VALUE;
  }
  return lhs + rhs;
#endif
}

static inline int64_t saturatingSub(int64_t lhs, int64_t rhs) {
#if defined(__GNUC__) || defined(__clang__)
  int64_t out = 0;
  if (!__builtin_sub_overflow(lhs, rhs, &out)) {
    return out;
  }
  return rhs >= 0 ? INT64_MIN_VALUE : INT64_MAX_VALUE;
#else
  if ((rhs > 0) && (lhs < (INT64_MIN_VALUE + rhs))) {
    return INT64_MIN_VALUE;
  }
  if ((rhs < 0) && (lhs > (INT64_MAX_VALUE + rhs))) {
    return INT64_MAX_VALUE;
  }
  return lhs - rhs;
#endif
}

static inline int64_t saturatingMul(int64_t lhs, int64_t rhs) {
#if defined(__GNUC__) || defined(__clang__)
  int64_t out = 0;
  if (!__builtin_mul_overflow(lhs, rhs, &out)) {
    return out;
  }
  const bool sameSign = (lhs < 0) == (rhs < 0);
  return sameSign ? INT64_MAX_VALUE : INT64_MIN_VALUE;
#else
  if ((lhs == 0) || (rhs == 0)) {
    return 0;
  }
  if ((lhs == -1) && (rhs == INT64_MIN_VALUE)) {
    return INT64_MAX_VALUE;
  }
  if ((rhs == -1) && (lhs == INT64_MIN_VALUE)) {
    return INT64_MAX_VALUE;
  }

  if (lhs > 0) {
    if (rhs > 0) {
      if (lhs > (INT64_MAX_VALUE / rhs)) {
        return INT64_MAX_VALUE;
      }
    } else {
      if (rhs < (INT64_MIN_VALUE / lhs)) {
        return INT64_MIN_VALUE;
      }
    }
  } else {
    if (rhs > 0) {
      if (lhs < (INT64_MIN_VALUE / rhs)) {
        return INT64_MIN_VALUE;
      }
    } else {
      if (lhs < (INT64_MAX_VALUE / rhs)) {
        return INT64_MAX_VALUE;
      }
    }
  }
  return lhs * rhs;
#endif
}

static inline int64_t millisToMicrosSaturated(int64_t valueMs) {
  return saturatingMul(valueMs, 1000LL);
}

static inline int64_t secondsToMicrosSaturated(int64_t valueS) {
  return saturatingMul(valueS, 1000000LL);
}

static inline uint64_t absToUnsigned(int64_t value) {
  if (value >= 0) {
    return static_cast<uint64_t>(value);
  }
  if (value == INT64_MIN_VALUE) {
    return static_cast<uint64_t>(INT64_MAX_VALUE) + 1ULL;
  }
  return static_cast<uint64_t>(-value);
}

static inline int32_t sizeToDetail(size_t value) {
  static constexpr size_t DETAIL_MAX = static_cast<size_t>((std::numeric_limits<int32_t>::max)());
  return static_cast<int32_t>(value > DETAIL_MAX ? DETAIL_MAX : value);
}

}  // namespace

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
  return saturatingSub(micros64Impl(), startUs);
}

int64_t millisSince(int64_t startMs) {
  return saturatingSub(millis64(), startMs);
}

int64_t secondsSince(int64_t startS) {
  return saturatingSub(seconds64(), startS);
}

Status formatTimeTo(int64_t microsSinceBoot, char* out, size_t outLen) {
  if ((out == nullptr) || (outLen == 0U)) {
    return Status(Err::INVALID_CONFIG, 0, "Output buffer is null or empty");
  }

  out[0] = '\0';

  if (outLen < TIME_FORMAT_BUFFER_SIZE) {
    return Status(Err::INVALID_CONFIG,
                  static_cast<int32_t>(TIME_FORMAT_BUFFER_SIZE),
                  "Output buffer too small");
  }

  const bool negative = microsSinceBoot < 0;
  const uint64_t totalMs = absToUnsigned(microsSinceBoot) / 1000ULL;
  const uint64_t hours = totalMs / 3600000ULL;
  const uint64_t minutes = (totalMs / 60000ULL) % 60ULL;
  const uint64_t seconds = (totalMs / 1000ULL) % 60ULL;
  const uint64_t millis = totalMs % 1000ULL;

  const int written = snprintf(out,
                               outLen,
                               "%s%llu:%02llu:%02llu.%03llu",
                               negative ? "-" : "",
                               static_cast<unsigned long long>(hours),
                               static_cast<unsigned long long>(minutes),
                               static_cast<unsigned long long>(seconds),
                               static_cast<unsigned long long>(millis));

  if (written < 0) {
    out[0] = '\0';
    return Status(Err::INTERNAL_ERROR, written, "Time formatting failed");
  }

  if (static_cast<size_t>(written) >= outLen) {
    out[0] = '\0';
    const size_t required = static_cast<size_t>(written) + 1U;
    return Status(Err::INVALID_CONFIG, sizeToDetail(required), "Output buffer too small");
  }

  return Ok();
}

Status formatNowTo(char* out, size_t outLen) {
  return formatTimeTo(micros64Impl(), out, outLen);
}

String formatTime(int64_t microsSinceBoot) {
  char buf[TIME_FORMAT_BUFFER_SIZE];
  const Status status = formatTimeTo(microsSinceBoot, buf, sizeof(buf));
  if (!status.ok()) {
    return String("");
  }
  return String(buf);
}

String formatNow() {
  return formatTime(micros64Impl());
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
    _totalUs = saturatingAdd(_totalUs, microsSince(_startUs));
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
    acc = saturatingAdd(acc, microsSince(_startUs));
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
  _us = saturatingSub(micros64Impl(), valUs);
}

ElapsedMicros64::ElapsedMicros64(const ElapsedMicros64& orig) {
  _us = orig._us;
}

ElapsedMicros64::operator int64_t() const {
  return saturatingSub(micros64Impl(), _us);
}

ElapsedMicros64& ElapsedMicros64::operator=(const ElapsedMicros64& rhs) {
  _us = rhs._us;
  return *this;
}

ElapsedMicros64& ElapsedMicros64::operator=(int64_t valUs) {
  _us = saturatingSub(micros64Impl(), valUs);
  return *this;
}

ElapsedMicros64& ElapsedMicros64::operator-=(int64_t valUs) {
  _us = saturatingAdd(_us, valUs);
  return *this;
}

ElapsedMicros64& ElapsedMicros64::operator+=(int64_t valUs) {
  _us = saturatingSub(_us, valUs);
  return *this;
}

ElapsedMicros64 ElapsedMicros64::operator-(int64_t valUs) const {
  ElapsedMicros64 r(*this);
  r._us = saturatingAdd(r._us, valUs);
  return r;
}

ElapsedMicros64 ElapsedMicros64::operator+(int64_t valUs) const {
  ElapsedMicros64 r(*this);
  r._us = saturatingSub(r._us, valUs);
  return r;
}

// ===========================================================================
// ElapsedMillis64 Implementation
// ===========================================================================

ElapsedMillis64::ElapsedMillis64() {
  _us = micros64Impl();
}

ElapsedMillis64::ElapsedMillis64(int64_t valMs) {
  _us = saturatingSub(micros64Impl(), millisToMicrosSaturated(valMs));
}

ElapsedMillis64::ElapsedMillis64(const ElapsedMillis64& orig) {
  _us = orig._us;
}

ElapsedMillis64::operator int64_t() const {
  return saturatingSub(micros64Impl(), _us) / 1000LL;
}

ElapsedMillis64& ElapsedMillis64::operator=(const ElapsedMillis64& rhs) {
  _us = rhs._us;
  return *this;
}

ElapsedMillis64& ElapsedMillis64::operator=(int64_t valMs) {
  _us = saturatingSub(micros64Impl(), millisToMicrosSaturated(valMs));
  return *this;
}

ElapsedMillis64& ElapsedMillis64::operator-=(int64_t valMs) {
  _us = saturatingAdd(_us, millisToMicrosSaturated(valMs));
  return *this;
}

ElapsedMillis64& ElapsedMillis64::operator+=(int64_t valMs) {
  _us = saturatingSub(_us, millisToMicrosSaturated(valMs));
  return *this;
}

ElapsedMillis64 ElapsedMillis64::operator-(int64_t valMs) const {
  ElapsedMillis64 r(*this);
  r._us = saturatingAdd(r._us, millisToMicrosSaturated(valMs));
  return r;
}

ElapsedMillis64 ElapsedMillis64::operator+(int64_t valMs) const {
  ElapsedMillis64 r(*this);
  r._us = saturatingSub(r._us, millisToMicrosSaturated(valMs));
  return r;
}

// ===========================================================================
// ElapsedSeconds64 Implementation
// ===========================================================================

ElapsedSeconds64::ElapsedSeconds64() {
  _us = micros64Impl();
}

ElapsedSeconds64::ElapsedSeconds64(int64_t valS) {
  _us = saturatingSub(micros64Impl(), secondsToMicrosSaturated(valS));
}

ElapsedSeconds64::ElapsedSeconds64(const ElapsedSeconds64& orig) {
  _us = orig._us;
}

ElapsedSeconds64::operator int64_t() const {
  return saturatingSub(micros64Impl(), _us) / 1000000LL;
}

ElapsedSeconds64& ElapsedSeconds64::operator=(const ElapsedSeconds64& rhs) {
  _us = rhs._us;
  return *this;
}

ElapsedSeconds64& ElapsedSeconds64::operator=(int64_t valS) {
  _us = saturatingSub(micros64Impl(), secondsToMicrosSaturated(valS));
  return *this;
}

ElapsedSeconds64& ElapsedSeconds64::operator-=(int64_t valS) {
  _us = saturatingAdd(_us, secondsToMicrosSaturated(valS));
  return *this;
}

ElapsedSeconds64& ElapsedSeconds64::operator+=(int64_t valS) {
  _us = saturatingSub(_us, secondsToMicrosSaturated(valS));
  return *this;
}

ElapsedSeconds64 ElapsedSeconds64::operator-(int64_t valS) const {
  ElapsedSeconds64 r(*this);
  r._us = saturatingAdd(r._us, secondsToMicrosSaturated(valS));
  return r;
}

ElapsedSeconds64 ElapsedSeconds64::operator+(int64_t valS) const {
  ElapsedSeconds64 r(*this);
  r._us = saturatingSub(r._us, secondsToMicrosSaturated(valS));
  return r;
}

}  // namespace SystemChrono
