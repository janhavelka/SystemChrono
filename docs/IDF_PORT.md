# SystemChrono ESP-IDF v6.0.1 Port Readiness Audit

Date: 2026-05-17.
Scope: documentation for a future ESP-IDF port only. Do not change code,
`library.json`, `README.md`, `CHANGELOG.md`, generated files, examples, or
tests while applying this audit.

## Current State

- `SystemChrono` provides 64-bit monotonic time helpers, elapsed timer classes,
  `Stopwatch`, and allocation-free formatters.
- ESP32 Arduino builds already use `esp_timer_get_time()` for `micros64()`.
- Public header `include/SystemChrono/SystemChrono.h` includes `<Arduino.h>`
  and exposes Arduino `String` return APIs: `formatTime()` and `formatNow()`.
- `src/SystemChrono.cpp` hard-blocks non-Arduino builds with `#error`.
- The deterministic APIs already exist: `formatTimeTo()`, `formatNowTo()`,
  fixed caller buffers, no hardware ownership, no tasks.
- Some public/project docs still describe the library as header-only. That is
  stale for the current repository; the implementation lives in
  `src/SystemChrono.cpp` and the IDF component must compile that source.
- `platformio.ini` and `library.json` are Arduino-only. There is no root
  `CMakeLists.txt`, `idf_component.yml`, or IDF example.

## Blockers

- Pure IDF cannot include the public header because of unconditional
  `<Arduino.h>`.
- Pure IDF cannot compile the implementation because of the non-Arduino
  `#error`.
- Arduino `String` in the public API is a heap-risk and an IDF compile blocker.
- Generic Arduino fallback uses `micros()`, `noInterrupts()`, and
  `interrupts()`; these must stay Arduino-only.
- No component metadata declares the `esp_timer` dependency.

## Exact Files and APIs to Change

- `include/SystemChrono/SystemChrono.h`
  - Remove unconditional `<Arduino.h>`.
  - Include only standard headers needed by all builds: `<stddef.h>`,
    `<stdint.h>`.
  - Keep `micros64()`, `millis64()`, `seconds64()`, elapsed helpers, timer
    classes, `formatTimeTo()`, and `formatNowTo()` unchanged.
  - Put `String formatTime()` and `String formatNow()` behind
    `#if defined(ARDUINO)` or move them to an Arduino compatibility header.
- `src/SystemChrono.cpp`
  - Remove the non-Arduino `#error`.
  - Add pure IDF path: include `<esp_timer.h>` and return
    `esp_timer_get_time()` from the platform microsecond source.
  - Keep generic Arduino wrap-tracking only under `#if defined(ARDUINO)`.
  - Compile `String` implementations only under Arduino.
- `include/SystemChrono/Config.h`
  - No required API change. Clean up stale comments that imply the library is
    header-only, then keep the file reserved unless a future injected clock
    source is needed.
- Build/example files to add later:
  - `CMakeLists.txt`
  - `idf_component.yml`
  - `examples/espidf_basic/`

## Compatibility Architecture

- Preserve Arduino source compatibility by keeping `String` helpers available
  only for Arduino builds.
- The pure-IDF API surface excludes Arduino `String` helpers unless they are
  moved to a separate Arduino compatibility header. IDF callers use
  `formatTimeTo()` and `formatNowTo()` with caller-provided buffers.
- Make allocation-free functions the cross-platform contract:
  - `formatTimeTo()`
  - `formatNowTo()`
  - elapsed timer classes
  - `Stopwatch`
- Use compile-time platform selection:
  - IDF: `esp_timer_get_time()`.
  - Arduino ESP32: `esp_timer_get_time()` through Arduino-ESP32.
  - Other Arduino: existing wrap-tracked `micros()`.
- Do not add background tasks, heap allocation, global hardware ownership, or
  logging.

## Adapter Contract

- Time source:
  - IDF `micros64()` returns `static_cast<int64_t>(esp_timer_get_time())`.
  - `millis64()` and `seconds64()` derive by integer division.
- FreeRTOS mapping:
  - The library does not delay or schedule.
  - IDF examples may use `vTaskDelay(pdMS_TO_TICKS(...))` outside the library.
- Formatting:
  - IDF callers must use caller-provided buffers with
    `TIME_FORMAT_BUFFER_SIZE`.
  - `String` helpers are Arduino-only compatibility APIs.
- Error handling:
  - Formatting failures continue returning `SystemChrono::Status` with static
    messages only.

## CMake and Component Plan

```cmake
idf_component_register(
  SRCS "src/SystemChrono.cpp"
  INCLUDE_DIRS "include"
  REQUIRES esp_timer
)
target_compile_definitions(${COMPONENT_LIB} PUBLIC SYSTEMCHRONO_PLATFORM_IDF=1)
```

```yaml
version: "1.2.0"
description: "64-bit monotonic time helpers"
targets:
  - esp32s2
  - esp32s3
dependencies:
  idf: ">=6.0.1"
```

Keep PlatformIO Arduino builds separate and continue generating `Version.h`
from the existing release process.

## Example Plan

- IDF example:
  - `examples/espidf_basic/main/main.cpp` with `app_main()`.
  - Print `micros64()`, `millis64()`, `seconds64()`.
  - Demonstrate `ElapsedMillis64`, `Stopwatch`, and `formatNowTo()`.
  - Sleep in the example loop with `vTaskDelay(pdMS_TO_TICKS(1000))`.
- Arduino example:
  - Keep existing CLI example compiling.
  - Confirm `formatTime()` and `formatNow()` remain available in Arduino mode.

## Test And Validation Plan

- Host/unit:
  - add a native test environment first; the current `platformio.ini` only
    covers Arduino example builds.
  - add a fake time source or controllable platform shim before relying on host
    tests for timer-class behavior.
  - Saturating add/sub/multiply edge cases through public timer operations.
  - `formatTimeTo()` success, null buffer, short buffer, negative values, and
    large hour counts.
  - `Stopwatch` start/stop/resume/reset behavior with a fake time source if one
    is added for tests.
- IDF build:
  - Component and example build for `esp32s2` and `esp32s3`.
- Arduino build:
  - Existing example builds for S2/S3.
- Runtime:
  - Verify `millis64()` monotonicity over repeated FreeRTOS delays.

## ESP-IDF v6.0.1 Hazards

- `esp_timer_get_time()` returns `int64_t` microseconds; keep all public 64-bit
  APIs signed and avoid truncation except where examples display values.
- Do not include `<Arduino.h>` in headers used by pure IDF.
- Do not use Arduino `String` in the IDF ABI.
- FreeRTOS delays belong in examples or applications, not this timing library.
- Avoid heap in formatting; `snprintf()` into caller buffers is acceptable.

## Ordered Checklist

1. Remove unconditional Arduino include from the public header.
2. Guard or move Arduino `String` APIs.
3. Remove the non-Arduino `#error`.
4. Add pure IDF `esp_timer_get_time()` platform path.
5. Add root `CMakeLists.txt` and `idf_component.yml`.
6. Add a minimal IDF example.
7. Build IDF component/examples for `esp32s2` and `esp32s3`.
8. Build existing Arduino examples.
9. Run formatter and elapsed-time unit tests.
