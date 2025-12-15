#include "SystemChrono.h"
#include <stdio.h>

#if !defined(ARDUINO)
  #error "SystemChrono: this library currently supports Arduino builds only."
#endif

#if defined(ARDUINO_ARCH_ESP32)
  #include "esp_timer.h"
#endif

namespace SystemChrono {

// ---------- internal: platform microsecond source ----------
static inline int64_t micros64_impl() {
#if defined(ARDUINO_ARCH_ESP32)
  // ESP32: monotonic microseconds since boot
  return (int64_t)esp_timer_get_time();

#elif defined(ARDUINO)
  // Generic Arduino: extend (typically) 32-bit micros() to 64-bit via wrap tracking.
  // Note: If micros() is already 64-bit on a given core, this still works.
  static uint32_t last = 0;
  static uint64_t high = 0;

  noInterrupts();
  uint32_t now = (uint32_t)micros();
  if (now < last) {
    high += (1ULL << 32);  // wrapped
  }
  last = now;
  uint64_t full = high | (uint64_t)now;
  interrupts();

  return (int64_t)full;

#else
  #error "SystemChrono: unsupported platform."
#endif
}

// ---------- public time accessors ----------
int64_t micros64() {
  return micros64_impl();
}

int64_t millis64() {
  return micros64_impl() / 1000ULL;
}

int64_t seconds64() {
  return micros64_impl() / 1000000ULL;
}

int64_t microsSince(int64_t start_us) {
  return micros64_impl() - start_us;
}

int64_t millisSince(int64_t start_ms) {
  return millis64() - start_ms;
}

int64_t secondsSince(int64_t start_s) {
  return seconds64() - start_s;
}

String formatTime(int64_t micros_since_boot) {
  int64_t total_ms = micros_since_boot / 1000LL;
  bool negative = total_ms < 0;
  if (negative) {
    total_ms = -total_ms;
  }

  int64_t hours = total_ms / 3600000LL;
  int64_t minutes = (total_ms / 60000LL) % 60LL;
  int64_t seconds = (total_ms / 1000LL) % 60LL;
  int64_t millis = total_ms % 1000LL;

  char buf[32];
  snprintf(buf, sizeof(buf), "%s%lld:%02lld:%02lld.%03lld",
           negative ? "-" : "",
           (long long)hours,
           (long long)minutes,
           (long long)seconds,
           (long long)millis);
  return String(buf);
}

String formatNow() {
  return formatTime(micros64());
}

// ================= elapsedMicros64 =================

elapsedMicros64::elapsedMicros64() { us_ = micros64_impl(); }

elapsedMicros64::elapsedMicros64(int64_t val_us) {
  us_ = micros64_impl() - val_us;
}

elapsedMicros64::elapsedMicros64(const elapsedMicros64& orig) {
  us_ = orig.us_;
}

elapsedMicros64::operator int64_t() const {
  return micros64_impl() - us_;
}

elapsedMicros64& elapsedMicros64::operator=(const elapsedMicros64& rhs) {
  us_ = rhs.us_;
  return *this;
}

elapsedMicros64& elapsedMicros64::operator=(int64_t val_us) {
  us_ = micros64_impl() - val_us;
  return *this;
}

elapsedMicros64& elapsedMicros64::operator-=(int64_t val_us) {
  us_ += val_us;
  return *this;
}

elapsedMicros64& elapsedMicros64::operator+=(int64_t val_us) {
  us_ -= val_us;
  return *this;
}

elapsedMicros64 elapsedMicros64::operator-(int64_t val_us) const {
  elapsedMicros64 r(*this);
  r.us_ += val_us;
  return r;
}

elapsedMicros64 elapsedMicros64::operator+(int64_t val_us) const {
  elapsedMicros64 r(*this);
  r.us_ -= val_us;
  return r;
}

// ================= elapsedMillis64 =================

elapsedMillis64::elapsedMillis64() { us_ = micros64_impl(); }

elapsedMillis64::elapsedMillis64(int64_t val_ms) {
  us_ = micros64_impl() - val_ms * 1000ULL;
}

elapsedMillis64::elapsedMillis64(const elapsedMillis64& orig) {
  us_ = orig.us_;
}

elapsedMillis64::operator int64_t() const {
  return (micros64_impl() - us_) / 1000ULL;
}

elapsedMillis64& elapsedMillis64::operator=(const elapsedMillis64& rhs) {
  us_ = rhs.us_;
  return *this;
}

elapsedMillis64& elapsedMillis64::operator=(int64_t val_ms) {
  us_ = micros64_impl() - val_ms * 1000ULL;
  return *this;
}

elapsedMillis64& elapsedMillis64::operator-=(int64_t val_ms) {
  us_ += val_ms * 1000ULL;
  return *this;
}

elapsedMillis64& elapsedMillis64::operator+=(int64_t val_ms) {
  us_ -= val_ms * 1000ULL;
  return *this;
}

elapsedMillis64 elapsedMillis64::operator-(int64_t val_ms) const {
  elapsedMillis64 r(*this);
  r.us_ += val_ms * 1000ULL;
  return r;
}

elapsedMillis64 elapsedMillis64::operator+(int64_t val_ms) const {
  elapsedMillis64 r(*this);
  r.us_ -= val_ms * 1000ULL;
  return r;
}

// ================= elapsedSeconds64 =================

elapsedSeconds64::elapsedSeconds64() { us_ = micros64_impl(); }

elapsedSeconds64::elapsedSeconds64(int64_t val_s) {
  us_ = micros64_impl() - val_s * 1000000ULL;
}

elapsedSeconds64::elapsedSeconds64(const elapsedSeconds64& orig) {
  us_ = orig.us_;
}

elapsedSeconds64::operator int64_t() const {
  return (micros64_impl() - us_) / 1000000ULL;
}

elapsedSeconds64& elapsedSeconds64::operator=(const elapsedSeconds64& rhs) {
  us_ = rhs.us_;
  return *this;
}

elapsedSeconds64& elapsedSeconds64::operator=(int64_t val_s) {
  us_ = micros64_impl() - val_s * 1000000ULL;
  return *this;
}

elapsedSeconds64& elapsedSeconds64::operator-=(int64_t val_s) {
  us_ += val_s * 1000000ULL;
  return *this;
}

elapsedSeconds64& elapsedSeconds64::operator+=(int64_t val_s) {
  us_ -= val_s * 1000000ULL;
  return *this;
}

elapsedSeconds64 elapsedSeconds64::operator-(int64_t val_s) const {
  elapsedSeconds64 r(*this);
  r.us_ += val_s * 1000000ULL;
  return r;
}

elapsedSeconds64 elapsedSeconds64::operator+(int64_t val_s) const {
  elapsedSeconds64 r(*this);
  r.us_ -= val_s * 1000000ULL;
  return r;
}

} // namespace SystemChrono
