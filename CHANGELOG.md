# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.2.0] - 2026-03-01

### Added
- CLI commands `stamp` / `since` demonstrating `microsSince()`, `millisSince()`, `secondsSince()`.
- CLI command `uptime` demonstrating `ElapsedSeconds64` and `formatTime()` (String variant).

### Changed
- Unified bringup CLI and shared log output style for consistent diagnostics across repositories.
- Refreshed version metadata export in public headers.
- Heartbeat log now includes uptime seconds from `ElapsedSeconds64`.

## [1.1.0] - 2026-02-10

### Changed
- Restructured from porting_library to template structure
- Renamed classes to PascalCase: `ElapsedMicros64`, `ElapsedMillis64`, `ElapsedSeconds64`
- Added lowercase aliases for backward compatibility
- Renamed `Stopwatch::running()` to `Stopwatch::isRunning()` for consistency
- Added comprehensive Doxygen documentation

### Added
- Allocation-free formatting APIs: `formatTimeTo()` and `formatNowTo()`
- `TIME_FORMAT_BUFFER_SIZE` constant for deterministic caller buffer sizing

### Fixed
- Eliminated undefined signed-overflow behavior in elapsed timer arithmetic via saturating math
- Hardened time formatting for extreme negative values (including `INT64_MIN`)
- Fixed PlatformIO pre-build version generation so `Version.h` is created during builds

## [1.0.1] - 2026-02-06

### Changed
- Updated LICENSE copyright notice

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

[Unreleased]: https://github.com/janhavelka/SystemChrono/compare/v1.2.0...HEAD
[1.2.0]: https://github.com/janhavelka/SystemChrono/compare/v1.1.0...v1.2.0
[1.1.0]: https://github.com/janhavelka/SystemChrono/compare/v1.0.1...v1.1.0
[1.0.1]: https://github.com/janhavelka/SystemChrono/compare/v1.0.0...v1.0.1
[1.0.0]: https://github.com/janhavelka/SystemChrono/releases/tag/v1.0.0
