/**
 * @file SystemChrono.h
 * @brief 64-bit monotonic time helpers for Arduino.
 *
 * Provides `micros64()`, `millis64()`, `seconds64()` with wrap-safe elapsed
 * calculations, human-readable formatting, and elapsed timer classes that
 * avoid the ~70 minute wrap of 32-bit timers.
 *
 * Uses `esp_timer_get_time()` on ESP32 for true 64-bit monotonic time.
 * Falls back to wrap-tracked `micros()` on other Arduino platforms.
 *
 * @note This library is header-only for the API declarations. Implementation
 *       is in SystemChrono.cpp (compiled as part of the library).
 */

#pragma once

#include <Arduino.h>
#include <stdint.h>

namespace SystemChrono {

// ===========================================================================
// Global 64-bit Time Accessors
// ===========================================================================

/**
 * @brief Get current time in microseconds (64-bit).
 * @return Monotonic microseconds since boot.
 *
 * @note On ESP32, uses `esp_timer_get_time()` for true 64-bit precision.
 * @note On other Arduino platforms, extends 32-bit `micros()` via wrap tracking.
 * @note Thread-safe on ESP32. On other platforms, uses interrupt-disable briefly.
 */
int64_t micros64();

/**
 * @brief Get current time in milliseconds (64-bit).
 * @return Monotonic milliseconds since boot.
 *
 * @note Derived from micros64() / 1000.
 */
int64_t millis64();

/**
 * @brief Get current time in seconds (64-bit).
 * @return Monotonic seconds since boot.
 *
 * @note Derived from micros64() / 1000000.
 */
int64_t seconds64();

// ===========================================================================
// Elapsed Time Helpers
// ===========================================================================

/**
 * @brief Calculate elapsed microseconds since a stored timestamp.
 * @param startUs Start timestamp from micros64().
 * @return Elapsed microseconds.
 */
int64_t microsSince(int64_t startUs);

/**
 * @brief Calculate elapsed milliseconds since a stored timestamp.
 * @param startMs Start timestamp from millis64().
 * @return Elapsed milliseconds.
 */
int64_t millisSince(int64_t startMs);

/**
 * @brief Calculate elapsed seconds since a stored timestamp.
 * @param startS Start timestamp from seconds64().
 * @return Elapsed seconds.
 */
int64_t secondsSince(int64_t startS);

// ===========================================================================
// Human-Readable Formatting
// ===========================================================================

/**
 * @brief Format microseconds as HH:MM:SS.mmm string.
 * @param microsSinceBoot Timestamp in microseconds.
 * @return Formatted string (e.g., "01:23:45.678").
 *
 * @note Returns String object. For embedded use, consider fixed-size buffers.
 * @note Handles negative values with leading minus sign.
 */
String formatTime(int64_t microsSinceBoot);

/**
 * @brief Format current time as HH:MM:SS.mmm string.
 * @return Formatted string of current time since boot.
 */
String formatNow();

// ===========================================================================
// Stopwatch Class
// ===========================================================================

/**
 * @brief Simple stopwatch utility for timing code blocks.
 *
 * Provides start/stop/resume/reset operations with microsecond precision.
 *
 * Usage:
 * @code
 * SystemChrono::Stopwatch sw;
 * sw.start();
 * // ... do work ...
 * sw.stop();
 * Serial.printf("Elapsed: %lld ms\n", (long long)sw.elapsedMillis());
 * @endcode
 *
 * @note Not thread-safe. Intended for sketch-level use in single-threaded context.
 */
class Stopwatch {
 public:
  Stopwatch();

  /**
   * @brief Reset and start the stopwatch.
   *
   * Clears accumulated time and starts counting from zero.
   */
  void start();

  /**
   * @brief Stop the stopwatch and accumulate elapsed time.
   *
   * Does nothing if already stopped.
   */
  void stop();

  /**
   * @brief Resume without clearing accumulated time.
   *
   * Does nothing if already running.
   */
  void resume();

  /**
   * @brief Clear accumulated time.
   *
   * If running, restarts from zero. If stopped, clears to zero.
   */
  void reset();

  /**
   * @brief Get total elapsed time in microseconds.
   * @return Accumulated microseconds (includes current run if running).
   */
  int64_t elapsedMicros() const;

  /**
   * @brief Get total elapsed time in milliseconds.
   * @return Accumulated milliseconds.
   */
  int64_t elapsedMillis() const;

  /**
   * @brief Get total elapsed time in seconds.
   * @return Accumulated seconds.
   */
  int64_t elapsedSeconds() const;

  /**
   * @brief Check if stopwatch is currently running.
   * @return true if running.
   */
  bool isRunning() const;

 private:
  int64_t _startUs;
  int64_t _totalUs;
  bool _running;
};

// ===========================================================================
// Elapsed Timer Classes
// ===========================================================================

/**
 * @brief Auto-incrementing microsecond timer.
 *
 * Behaves like an int64_t that automatically increases based on elapsed time.
 * Useful for non-blocking interval checking.
 *
 * Usage:
 * @code
 * SystemChrono::ElapsedMicros64 timer;
 * // ... later ...
 * if (timer >= 1000000) {  // 1 second elapsed
 *   timer = 0;  // reset
 *   // do periodic work
 * }
 * @endcode
 *
 * @note Stores the start timestamp internally. Reading the value computes
 *       elapsed time from that stored timestamp.
 */
class ElapsedMicros64 {
 public:
  ElapsedMicros64();
  explicit ElapsedMicros64(int64_t valUs);
  ElapsedMicros64(const ElapsedMicros64& orig);

  operator int64_t() const;

  ElapsedMicros64& operator=(const ElapsedMicros64& rhs);
  ElapsedMicros64& operator=(int64_t valUs);

  ElapsedMicros64& operator-=(int64_t valUs);
  ElapsedMicros64& operator+=(int64_t valUs);

  ElapsedMicros64 operator-(int64_t valUs) const;
  ElapsedMicros64 operator+(int64_t valUs) const;

 private:
  int64_t _us;
};

/**
 * @brief Auto-incrementing millisecond timer.
 *
 * Same as ElapsedMicros64 but returns elapsed milliseconds.
 *
 * Usage:
 * @code
 * SystemChrono::ElapsedMillis64 heartbeat;
 * // ... later ...
 * if (heartbeat >= 1000) {  // 1 second elapsed
 *   heartbeat = 0;
 *   // do periodic work
 * }
 * @endcode
 */
class ElapsedMillis64 {
 public:
  ElapsedMillis64();
  explicit ElapsedMillis64(int64_t valMs);
  ElapsedMillis64(const ElapsedMillis64& orig);

  operator int64_t() const;

  ElapsedMillis64& operator=(const ElapsedMillis64& rhs);
  ElapsedMillis64& operator=(int64_t valMs);

  ElapsedMillis64& operator-=(int64_t valMs);
  ElapsedMillis64& operator+=(int64_t valMs);

  ElapsedMillis64 operator-(int64_t valMs) const;
  ElapsedMillis64 operator+(int64_t valMs) const;

 private:
  int64_t _us;
};

/**
 * @brief Auto-incrementing seconds timer.
 *
 * Same as ElapsedMicros64 but returns elapsed seconds.
 *
 * Usage:
 * @code
 * SystemChrono::ElapsedSeconds64 uptime;
 * // ... later ...
 * Serial.printf("Uptime: %lld seconds\n", (long long)uptime);
 * @endcode
 */
class ElapsedSeconds64 {
 public:
  ElapsedSeconds64();
  explicit ElapsedSeconds64(int64_t valS);
  ElapsedSeconds64(const ElapsedSeconds64& orig);

  operator int64_t() const;

  ElapsedSeconds64& operator=(const ElapsedSeconds64& rhs);
  ElapsedSeconds64& operator=(int64_t valS);

  ElapsedSeconds64& operator-=(int64_t valS);
  ElapsedSeconds64& operator+=(int64_t valS);

  ElapsedSeconds64 operator-(int64_t valS) const;
  ElapsedSeconds64 operator+(int64_t valS) const;

 private:
  int64_t _us;
};

// ===========================================================================
// Legacy Compatibility Aliases (lowercase)
// ===========================================================================

/// @brief Alias for ElapsedMicros64 (legacy compatibility).
using elapsedMicros64 = ElapsedMicros64;

/// @brief Alias for ElapsedMillis64 (legacy compatibility).
using elapsedMillis64 = ElapsedMillis64;

/// @brief Alias for ElapsedSeconds64 (legacy compatibility).
using elapsedSeconds64 = ElapsedSeconds64;

}  // namespace SystemChrono
