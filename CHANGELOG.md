# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [2.0.0] - 2026-01-12

### Added
- Optional lifecycle hooks: begin(), tick(), end()
- Buffer-based formatting APIs: formatTimeToBuffer(), formatNowToBuffer()
- Formatting buffer size constant: kFormatTimeBufferSize
- Native host test for formatting

### Changed
- Restructured from porting_library to template structure
- Renamed classes to PascalCase: `ElapsedMicros64`, `ElapsedMillis64`, `ElapsedSeconds64`
- Added lowercase aliases for backward compatibility
- Added comprehensive Doxygen documentation
- CLI example uses buffer formatting and tick()
- README updated with behavioral contracts and corrected project structure

### Removed
- Removed `Stopwatch::running()` (use `isRunning()`)

## [1.0.1] - 2026-01-10

### Fixed
- Minor documentation updates

## [1.0.0] - 2026-01-10

### Added
- Initial release
- 64-bit time accessors: `micros64()`, `millis64()`, `seconds64()`
- Elapsed helpers: `microsSince()`, `millisSince()`, `secondsSince()`
- Elapsed timer classes: `elapsedMicros64`, `elapsedMillis64`, `elapsedSeconds64`
- `Stopwatch` class with start/stop/resume/reset
- Human-readable formatting: `formatTime()`, `formatNow()`
- ESP32 optimized using `esp_timer_get_time()`
- Generic Arduino support with wrap-tracked `micros()`

[Unreleased]: https://github.com/janhavelka/SystemChrono/compare/v2.0.0...HEAD
[2.0.0]: https://github.com/janhavelka/SystemChrono/compare/v1.0.1...v2.0.0
[1.0.1]: https://github.com/janhavelka/SystemChrono/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/janhavelka/SystemChrono/releases/tag/v1.0.0
