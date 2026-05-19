# SystemChrono ESP-IDF v6.0.1 Port

Date: 2026-05-17.
Updated: 2026-05-19.
Scope: keep SystemChrono usable from both Arduino/PlatformIO and pure ESP-IDF
while preserving Arduino source compatibility where possible.

## Result

- Public headers no longer include `<Arduino.h>` for pure ESP-IDF builds.
- Arduino `String` helpers remain available only under `#if defined(ARDUINO)`.
- Cross-platform deterministic APIs remain unchanged: `micros64()`,
  `millis64()`, `seconds64()`, elapsed helpers, timer classes, `Stopwatch`,
  `formatTimeTo()`, and `formatNowTo()`.
- `src/SystemChrono.cpp` no longer blocks non-Arduino builds.
- Pure ESP-IDF builds use `esp_timer_get_time()` for the platform microsecond
  source.
- Root `CMakeLists.txt` and `idf_component.yml` make the library consumable as
  an ESP-IDF component.
- `examples/espidf_basic` shares the same colored interactive CLI source as the
  Arduino example while using pure ESP-IDF entry-point and console glue.

## Current State

- `SystemChrono` provides 64-bit monotonic time helpers, elapsed timer classes,
  `Stopwatch`, and allocation-free formatters.
- ESP32 Arduino and pure ESP-IDF builds use `esp_timer_get_time()` for
  `micros64()`.
- Public header `include/SystemChrono/SystemChrono.h` includes `<Arduino.h>`
  only under Arduino and exposes `String` return APIs only in Arduino builds.
- `src/SystemChrono.cpp` compiles for pure ESP-IDF and Arduino.
- The deterministic APIs already exist: `formatTimeTo()`, `formatNowTo()`,
  fixed caller buffers, no hardware ownership, no tasks.
- Public/project docs now describe the library as source-backed. The
  implementation lives in `src/SystemChrono.cpp` and the IDF component compiles
  that source.
- `platformio.ini` remains the Arduino example build entry point.
- `library.json` declares Arduino and ESP-IDF compatibility.
- Root `CMakeLists.txt`, `idf_component.yml`, and a native IDF example are
  present.

## Previous Blockers Resolved

- Pure IDF can include the public header without `<Arduino.h>`.
- Pure IDF can compile the implementation.
- Arduino `String` helpers are excluded from the IDF ABI.
- Generic Arduino fallback uses `micros()`, `noInterrupts()`, and
  `interrupts()` only under Arduino.
- Component metadata declares the `esp_timer` dependency.

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
- Build/example files:
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
  - `examples/espidf_basic/main/main.cpp` is a native `app_main()` program.
  - It uses FreeRTOS delays, nonblocking POSIX stdin, `esp_rom_delay_us`, and
    fixed C buffers directly.
  - It does not include Arduino sources and does not use a `Serial`/`millis`/
    `delay` compatibility facade.
  - The command names mirror the Arduino CLI: help/version/info/status/config,
    time/uptime/format/stamp/since/measure, and stopwatch controls.
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

## Validation

Completed locally:

- `python -m platformio run -e cli_esp32s3`
- `python -m platformio run -e cli_esp32s2`
- `python scripts/generate_version.py`
- `git diff --check`

Pending in this shell:

- `idf.py build` for the native CLI in `examples/espidf_basic`
- IDF target builds for `esp32s2` and `esp32s3`

`idf.py` was not available on PATH during this implementation pass, so the
ESP-IDF example is implemented and documented but still needs a real ESP-IDF
toolchain build before release.
