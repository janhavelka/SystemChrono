#pragma once

#include <Arduino.h>
#include <stdint.h>

namespace SystemChrono {

// ----------- Global 64-bit Time Accessors -----------
int64_t micros64();
int64_t millis64();
int64_t seconds64();

// Convenient elapsed calculations against a stored timestamp.
int64_t microsSince(int64_t start_us);
int64_t millisSince(int64_t start_ms);
int64_t secondsSince(int64_t start_s);

// Human-readable formatting: HH:MM:SS.mmm from microseconds since boot.
String formatTime(int64_t micros_since_boot);
String formatNow();

// Simple stopwatch utility (not thread-safe; intended for sketch-level use).
class Stopwatch {
private:
  int64_t start_us_;
  int64_t total_us_;
  bool running_;

public:
  Stopwatch();

  void start();   // Reset and start
  void stop();    // Stop and accumulate
  void resume();  // Resume without clearing accumulated time
  void reset();   // Clear accumulated time; keeps running state

  int64_t elapsedMicros() const;
  int64_t elapsedMillis() const;
  int64_t elapsedSeconds() const;

  bool running() const;
};

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

} // namespace SystemChrono
