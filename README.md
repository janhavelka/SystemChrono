# SystemChrono

64-bit monotonic time helpers for Arduino, using `esp_timer_get_time()` on ESP32 and rollover-tracked `micros()` elsewhere.

[![CI](https://github.com/janhavelka/SystemChrono/actions/workflows/ci.yml/badge.svg)](https://github.com/janhavelka/SystemChrono/actions/workflows/ci.yml)

## Features

- **64-bit time accessors:** `micros64()`, `millis64()`, `seconds64()` - no 70-minute rollover
- **Elapsed helpers:** `microsSince()`, `millisSince()`, `secondsSince()` for interval checks
- **Elapsed timer classes:** `ElapsedMicros64`, `ElapsedMillis64`, `ElapsedSeconds64` for non-blocking intervals
- **Stopwatch:** Start/stop/resume/reset with microsecond precision
- **Human-readable formatting:** allocation-free `formatTimeTo()` / `formatNowTo()` plus String wrappers
- **ESP32 optimized:** Uses `esp_timer_get_time()` for true 64-bit monotonic time
- **Arduino compatible:** Falls back to wrap-tracked `micros()` on other platforms

## Quickstart

```bash
# Clone
git clone https://github.com/janhavelka/SystemChrono.git
cd SystemChrono

# Build for ESP32-S3
pio run -e cli_esp32s3

# Upload and monitor
pio run -e cli_esp32s3 -t upload && pio device monitor -e cli_esp32s3
```

## Supported Targets

| Board                 | Environment           | Notes        |
| --------------------- | --------------------- | ------------ |
| ESP32-S3-DevKitC-1    | `cli_esp32s3`         | PSRAM enabled |
| ESP32-S2-Saola-1      | `cli_esp32s2`         | USB CDC      |

## Usage

### Basic Time Accessors

```cpp
#include "SystemChrono/SystemChrono.h"

using namespace SystemChrono;

void loop() {
  // 64-bit timestamps - no 70-minute rollover
  int64_t us = micros64();
  int64_t ms = millis64();
  int64_t s = seconds64();
  
  Serial.printf("micros=%lld millis=%lld seconds=%lld\n",
                (long long)us, (long long)ms, (long long)s);
}
```

### Elapsed Timer Classes

```cpp
#include "SystemChrono/SystemChrono.h"

using namespace SystemChrono;

ElapsedMillis64 heartbeat;

void loop() {
  // Non-blocking 1-second interval
  if (heartbeat >= 1000) {
    heartbeat = 0;  // Reset timer
    Serial.println("Tick!");
  }
}
```

### Stopwatch

```cpp
#include "SystemChrono/SystemChrono.h"

using namespace SystemChrono;

Stopwatch sw;

void measureSomething() {
  sw.start();
  
  // ... do work ...
  
  sw.stop();
  Serial.printf("Elapsed: %lld ms\n", (long long)sw.elapsedMillis());
}
```

### Human-Readable Formatting

```cpp
#include "SystemChrono/SystemChrono.h"

using namespace SystemChrono;

void printUptime() {
  char timeBuf[TIME_FORMAT_BUFFER_SIZE];

  // Deterministic, allocation-free formatting (recommended in production)
  if (formatNowTo(timeBuf, sizeof(timeBuf)).ok()) {
    Serial.println(timeBuf);
  }

  // String wrappers remain available for convenience
  Serial.println(formatNow());
}
```

## API Reference

### Free Functions

| Function                          | Description                                    |
| --------------------------------- | ---------------------------------------------- |
| `int64_t micros64()`              | Monotonic microseconds since boot              |
| `int64_t millis64()`              | Monotonic milliseconds since boot              |
| `int64_t seconds64()`             | Monotonic seconds since boot                   |
| `int64_t microsSince(int64_t)`    | Elapsed microseconds since timestamp           |
| `int64_t millisSince(int64_t)`    | Elapsed milliseconds since timestamp           |
| `int64_t secondsSince(int64_t)`   | Elapsed seconds since timestamp                |
| `Status formatTimeTo(int64_t, char*, size_t)` | Allocation-free format into caller buffer |
| `Status formatNowTo(char*, size_t)` | Allocation-free format into caller buffer    |
| `String formatTime(int64_t)`      | Format microseconds as `HH:MM:SS.mmm`          |
| `String formatNow()`              | Format current time as `HH:MM:SS.mmm`          |

### Stopwatch Class

| Method                    | Description                                |
| ------------------------- | ------------------------------------------ |
| `void start()`            | Reset and start                            |
| `void stop()`             | Stop and accumulate elapsed time           |
| `void resume()`           | Resume without clearing accumulated time   |
| `void reset()`            | Clear accumulated time                     |
| `int64_t elapsedMicros()` | Get accumulated microseconds               |
| `int64_t elapsedMillis()` | Get accumulated milliseconds               |
| `int64_t elapsedSeconds()`| Get accumulated seconds                    |
| `bool isRunning()`        | Check if currently running                 |

### Elapsed Timer Classes

All three classes (`ElapsedMicros64`, `ElapsedMillis64`, `ElapsedSeconds64`) support:

- Implicit conversion to `int64_t` (returns elapsed time)
- Assignment `= 0` to reset
- Arithmetic operators `+=`, `-=`, `+`, `-`

## Versioning

The library version is defined in [library.json](library.json). A pre-build script automatically generates `include/SystemChrono/Version.h`.

```cpp
#include "SystemChrono/Version.h"

Serial.println(SystemChrono::VERSION);           // "1.1.0"
Serial.println(SystemChrono::VERSION_FULL);      // "1.1.0 (a1b2c3d, 2026-02-10 15:30:00)"
Serial.println(SystemChrono::BUILD_TIMESTAMP);   // "2026-02-10 15:30:00"
Serial.println(SystemChrono::GIT_COMMIT);        // "a1b2c3d"
```

## Examples

| Example                  | Description                                      |
| ------------------------ | ------------------------------------------------ |
| `01_basic_bringup_cli`   | Interactive CLI demonstrating all features       |

### Building Examples

```bash
# CLI example (S3)
pio run -e cli_esp32s3 -t upload
pio device monitor -e cli_esp32s3

# CLI example (S2)
pio run -e cli_esp32s2 -t upload
pio device monitor -e cli_esp32s2
```

## Threading & Timing Model

- **Single-threaded:** All functions safe to call from main loop
- **Non-blocking:** No delays or waits
- **ISR safety (ESP32):** `micros64()` uses `esp_timer_get_time()` which is ISR-safe
- **ISR safety (other):** Uses `noInterrupts()`/`interrupts()` briefly for wrap tracking

## Resource Ownership

- No pins, buses, tasks, or peripherals are owned by this library
- Time source is platform-provided (`esp_timer_get_time()` or `micros()`)
- No hidden storage or NVS side effects

## Memory

- No heap allocations in `micros64()`, elapsed helpers, `Stopwatch`, or elapsed timer classes
- `formatTimeTo()` and `formatNowTo()` are allocation-free
- `formatTime()` and `formatNow()` return `String` and may allocate heap memory

## Error Handling

- Allocation-free formatting APIs return `Status` and never fail silently
- Invalid formatting buffer configuration returns `Err::INVALID_CONFIG`

## Platform Notes

### ESP32
Uses `esp_timer_get_time()` for true 64-bit monotonic microseconds since boot. Thread-safe.

### Other Arduino Platforms
Extends 32-bit `micros()` to 64-bit via wrap tracking. Requires periodic calls (at least once per ~70 minutes) to detect rollovers. Uses interrupt-disable briefly when reading.

## Project Structure

```
├── include/SystemChrono/  # Public headers (library API)
│   ├── Config.h          # Configuration struct (reserved)
│   ├── Status.h          # Error types
│   ├── SystemChrono.h    # Main API header
│   └── Version.h         # Auto-generated version info
├── src/                  # Implementation
│   └── SystemChrono.cpp
├── examples/
│   ├── 01_basic_bringup_cli/  # CLI demo
│   └── common/           # Shared example utilities
├── library.json          # PlatformIO library metadata
└── platformio.ini        # Build environments
```

## Versioning Policy

This project follows [Semantic Versioning 2.0.0](https://semver.org/):

- **MAJOR:** Breaking API changes
- **MINOR:** New features, backward compatible
- **PATCH:** Bug fixes, backward compatible

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

This project is licensed under the MIT License - see [LICENSE](LICENSE) for details.

## See Also

- [CHANGELOG.md](CHANGELOG.md) - Version history
- [SECURITY.md](SECURITY.md) - Security policy
- [AGENTS.md](AGENTS.md) - AI agent guidelines
