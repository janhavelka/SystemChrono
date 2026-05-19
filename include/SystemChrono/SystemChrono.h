/**
 * @file SystemChrono.h
 * @brief 64-bit monotonic time helpers for Arduino and ESP-IDF.
 *
 * Provides `micros64()`, `millis64()`, `seconds64()` with wrap-safe elapsed
 * calculations, human-readable formatting, and elapsed timer classes that
 * avoid the ~70 minute wrap of 32-bit timers.
 *
 * Uses `esp_timer_get_time()` on ESP32/ESP-IDF for true 64-bit monotonic time.
 * Falls back to wrap-tracked `micros()` on other Arduino platforms.
 *
 * @note Public declarations live in this header. Implementation is in
 *       SystemChrono.cpp and must be compiled as part of the library/component.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#if defined(ARDUINO)
#include <Arduino.h>
#endif

#include "SystemChrono/Status.h"

namespace SystemChrono {

// ===========================================================================
// Global 64-bit Time Accessors
// ===========================================================================

/**
 * @brief Get current time in microseconds (64-bit).
 * @return Monotonic microseconds since boot.
 *
 * @note On ESP32/ESP-IDF, uses `esp_timer_get_time()` for true 64-bit precision.
 * @note On other Arduino platforms, extends 32-bit `micros()` via wrap tracking.
 *       Call at least once per 32-bit micros() wrap period on those platforms.
 * @note Thread-safe on ESP32/ESP-IDF. On other Arduino platforms, uses
 *       interrupt-disable briefly.
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
 * @brief Minimum buffer size for time formatting functions.
 *
 * This size guarantees enough room for the longest supported
 * `[-]HHHHHHHHHHHHH:MM:SS.mmm` representation plus null terminator.
 */
static constexpr size_t TIME_FORMAT_BUFFER_SIZE = 32U;

/**
 * @brief Format microseconds as HH:MM:SS.mmm into caller-provided buffer.
 * @param microsSinceBoot Timestamp in microseconds.
 * @param out Output buffer for null-terminated formatted string.
 * @param outLen Size of output buffer in bytes.
 * @return OK on success.
 * @return INVALID_CONFIG if `out` is null, `outLen == 0`, or buffer too small.
 *
 * @note Deterministic and allocation-free. Preferred for production firmware.
 */
Status formatTimeTo(int64_t microsSinceBoot, char* out, size_t outLen);

/**
 * @brief Format current time as HH:MM:SS.mmm into caller-provided buffer.
 * @param out Output buffer for null-terminated formatted string.
 * @param outLen Size of output buffer in bytes.
 * @return OK on success.
 * @return INVALID_CONFIG if `out` is null, `outLen == 0`, or buffer too small.
 *
 * @note Deterministic and allocation-free. Preferred for production firmware.
 */
Status formatNowTo(char* out, size_t outLen);

#if defined(ARDUINO)
/**
 * @brief Format microseconds as HH:MM:SS.mmm string.
 * @param microsSinceBoot Timestamp in microseconds.
 * @return Formatted string (e.g., "01:23:45.678").
 *
 * @note Returns String object (heap allocation possible).
 * @note For deterministic memory usage, prefer formatTimeTo().
 * @note Handles negative values with leading minus sign.
 */
String formatTime(int64_t microsSinceBoot);

/**
 * @brief Format current time as HH:MM:SS.mmm string.
 * @return Formatted string of current time since boot.
 * @note Returns String object (heap allocation possible).
 * @note For deterministic memory usage, prefer formatNowTo().
 */
String formatNow();
#endif

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
  /// @brief Construct a stopped stopwatch with zero accumulated time.
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
  /// @brief Construct a timer initialized to zero elapsed microseconds.
  ElapsedMicros64();

  /// @brief Construct a timer with an initial elapsed value.
  /// @param valUs Initial elapsed microseconds.
  explicit ElapsedMicros64(int64_t valUs);

  /// @brief Copy constructor preserving the source timer baseline.
  /// @param orig Timer to copy.
  ElapsedMicros64(const ElapsedMicros64& orig);

  /// @brief Read elapsed microseconds.
  /// @return Elapsed microseconds since construction or last assignment.
  operator int64_t() const;

  /// @brief Copy timer baseline from another timer.
  /// @param rhs Source timer.
  /// @return Reference to this timer.
  ElapsedMicros64& operator=(const ElapsedMicros64& rhs);

  /// @brief Reset timer to a specific elapsed value.
  /// @param valUs New elapsed microseconds.
  /// @return Reference to this timer.
  ElapsedMicros64& operator=(int64_t valUs);

  /// @brief Reduce the reported elapsed value.
  /// @param valUs Microseconds to subtract from the elapsed reading.
  /// @return Reference to this timer.
  ElapsedMicros64& operator-=(int64_t valUs);

  /// @brief Increase the reported elapsed value.
  /// @param valUs Microseconds to add to the elapsed reading.
  /// @return Reference to this timer.
  ElapsedMicros64& operator+=(int64_t valUs);

  /// @brief Return a timer whose reported elapsed value is reduced.
  /// @param valUs Microseconds to subtract from the elapsed reading.
  /// @return Adjusted timer copy.
  ElapsedMicros64 operator-(int64_t valUs) const;

  /// @brief Return a timer whose reported elapsed value is increased.
  /// @param valUs Microseconds to add to the elapsed reading.
  /// @return Adjusted timer copy.
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
  /// @brief Construct a timer initialized to zero elapsed milliseconds.
  ElapsedMillis64();

  /// @brief Construct a timer with an initial elapsed value.
  /// @param valMs Initial elapsed milliseconds.
  explicit ElapsedMillis64(int64_t valMs);

  /// @brief Copy constructor preserving the source timer baseline.
  /// @param orig Timer to copy.
  ElapsedMillis64(const ElapsedMillis64& orig);

  /// @brief Read elapsed milliseconds.
  /// @return Elapsed milliseconds since construction or last assignment.
  operator int64_t() const;

  /// @brief Copy timer baseline from another timer.
  /// @param rhs Source timer.
  /// @return Reference to this timer.
  ElapsedMillis64& operator=(const ElapsedMillis64& rhs);

  /// @brief Reset timer to a specific elapsed value.
  /// @param valMs New elapsed milliseconds.
  /// @return Reference to this timer.
  ElapsedMillis64& operator=(int64_t valMs);

  /// @brief Reduce the reported elapsed value.
  /// @param valMs Milliseconds to subtract from the elapsed reading.
  /// @return Reference to this timer.
  ElapsedMillis64& operator-=(int64_t valMs);

  /// @brief Increase the reported elapsed value.
  /// @param valMs Milliseconds to add to the elapsed reading.
  /// @return Reference to this timer.
  ElapsedMillis64& operator+=(int64_t valMs);

  /// @brief Return a timer whose reported elapsed value is reduced.
  /// @param valMs Milliseconds to subtract from the elapsed reading.
  /// @return Adjusted timer copy.
  ElapsedMillis64 operator-(int64_t valMs) const;

  /// @brief Return a timer whose reported elapsed value is increased.
  /// @param valMs Milliseconds to add to the elapsed reading.
  /// @return Adjusted timer copy.
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
  /// @brief Construct a timer initialized to zero elapsed seconds.
  ElapsedSeconds64();

  /// @brief Construct a timer with an initial elapsed value.
  /// @param valS Initial elapsed seconds.
  explicit ElapsedSeconds64(int64_t valS);

  /// @brief Copy constructor preserving the source timer baseline.
  /// @param orig Timer to copy.
  ElapsedSeconds64(const ElapsedSeconds64& orig);

  /// @brief Read elapsed seconds.
  /// @return Elapsed seconds since construction or last assignment.
  operator int64_t() const;

  /// @brief Copy timer baseline from another timer.
  /// @param rhs Source timer.
  /// @return Reference to this timer.
  ElapsedSeconds64& operator=(const ElapsedSeconds64& rhs);

  /// @brief Reset timer to a specific elapsed value.
  /// @param valS New elapsed seconds.
  /// @return Reference to this timer.
  ElapsedSeconds64& operator=(int64_t valS);

  /// @brief Reduce the reported elapsed value.
  /// @param valS Seconds to subtract from the elapsed reading.
  /// @return Reference to this timer.
  ElapsedSeconds64& operator-=(int64_t valS);

  /// @brief Increase the reported elapsed value.
  /// @param valS Seconds to add to the elapsed reading.
  /// @return Reference to this timer.
  ElapsedSeconds64& operator+=(int64_t valS);

  /// @brief Return a timer whose reported elapsed value is reduced.
  /// @param valS Seconds to subtract from the elapsed reading.
  /// @return Adjusted timer copy.
  ElapsedSeconds64 operator-(int64_t valS) const;

  /// @brief Return a timer whose reported elapsed value is increased.
  /// @param valS Seconds to add to the elapsed reading.
  /// @return Adjusted timer copy.
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
