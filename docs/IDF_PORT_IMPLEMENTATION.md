# SystemChrono ESP-IDF Port Implementation Notes

Date: 2026-05-19.
Branch: `feature/systemchrono-idf-port`.

## Scope

- Kept the deterministic, allocation-free API surface unchanged for Arduino and
  ESP-IDF callers.
- Removed Arduino from the pure-IDF public header path.
- Kept Arduino `String` convenience wrappers available only in Arduino builds.
- Added ESP-IDF component metadata and a native IDF example that shares the
  same colored interactive CLI source as the Arduino example.

## Files Added

- `CMakeLists.txt`
- `idf_component.yml`
- `examples/espidf_basic/CMakeLists.txt`
- `examples/espidf_basic/main/CMakeLists.txt`
- `examples/espidf_basic/main/main.cpp`
- `scripts/check_idf_example_contract.py`

## Audit Resolution

- Public header compile blocker:
  - `SystemChrono.h` now includes `<stddef.h>` and `<stdint.h>` for all builds
    and includes `<Arduino.h>` only when `ARDUINO` is defined.
- Arduino `String` ABI blocker:
  - `formatTime()` and `formatNow()` declarations and definitions are guarded by
    `#if defined(ARDUINO)`.
  - Pure IDF callers use `formatTimeTo()` and `formatNowTo()`.
- Non-Arduino implementation blocker:
  - Removed the top-level non-Arduino `#error`.
  - Added an `ESP_PLATFORM` platform path using `esp_timer_get_time()`.
  - Kept wrap-tracked `micros()` and interrupt guards only under Arduino.
- Component metadata:
  - The root component compiles `src/SystemChrono.cpp`, exports `include/`, and
    declares `esp_timer`.
- Native IDF example:
  - `app_main()` uses native ESP-IDF and POSIX APIs directly: FreeRTOS delays,
    nonblocking `STDIN_FILENO`, `esp_rom_delay_us`, and fixed C buffers.
  - ESP-IDF exposes the same command names as the Arduino CLI for time,
    formatting, stamp/since, and stopwatch diagnostics without including
    Arduino source or compatibility facades.
  - `scripts/check_idf_example_contract.py` statically guards required CMake
    dependencies, native-IDF tokens, forbidden Arduino facade tokens, and the
    CLI command surface.

## Remaining Hardware Checks

- Build `examples/espidf_basic` with ESP-IDF v6.0.1 for `esp32s3` and `esp32s2`;
  `idf.py` was not available on PATH in this shell.
- Verify repeated `millis64()` monotonicity over FreeRTOS delays on hardware.

## Verification

- `python -m platformio run -e cli_esp32s3`: passed.
- `python -m platformio run -e cli_esp32s2`: passed.
- `python scripts/generate_version.py`: passed; generated `Version.h` was not committed.
- `git diff --check`: passed.
