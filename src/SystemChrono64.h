#pragma once

#include <Arduino.h>
#include <stdint.h>

namespace SystemChrono64 {

// ----------- Global 64-bit Time Accessors -----------
int64_t micros64();
int64_t millis64();

class elapsedMicros64 {
private:
  int64_t us_;

public:
  elapsedMicros64();
  explicit elapsedMicros64(int64_t val_us);
  elapsedMicros64(const elapsedMicros64& orig);

  operator int64_t() const;

  elapsedMicros64& operator=(const elapsedMicros64& rhs);
  elapsedMicros64& operator=(int64_t val_us);

  elapsedMicros64& operator-=(int64_t val_us);
  elapsedMicros64& operator+=(int64_t val_us);

  elapsedMicros64 operator-(int64_t val_us) const;
  elapsedMicros64 operator+(int64_t val_us) const;
};

class elapsedMillis64 {
private:
  int64_t us_;

public:
  elapsedMillis64();
  explicit elapsedMillis64(int64_t val_ms);
  elapsedMillis64(const elapsedMillis64& orig);

  operator int64_t() const;

  elapsedMillis64& operator=(const elapsedMillis64& rhs);
  elapsedMillis64& operator=(int64_t val_ms);

  elapsedMillis64& operator-=(int64_t val_ms);
  elapsedMillis64& operator+=(int64_t val_ms);

  elapsedMillis64 operator-(int64_t val_ms) const;
  elapsedMillis64 operator+(int64_t val_ms) const;
};

class elapsedSeconds64 {
private:
  int64_t us_;

public:
  elapsedSeconds64();
  explicit elapsedSeconds64(int64_t val_s);
  elapsedSeconds64(const elapsedSeconds64& orig);

  operator int64_t() const;

  elapsedSeconds64& operator=(const elapsedSeconds64& rhs);
  elapsedSeconds64& operator=(int64_t val_s);

  elapsedSeconds64& operator-=(int64_t val_s);
  elapsedSeconds64& operator+=(int64_t val_s);

  elapsedSeconds64 operator-(int64_t val_s) const;
  elapsedSeconds64 operator+(int64_t val_s) const;
};

} // namespace SystemChrono64

