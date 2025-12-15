# SystemChrono

64-bit monotonic time helpers for Arduino, using `esp_timer_get_time()` on ESP32 and rollover-tracked `micros()` elsewhere. Provides `micros64()`, `millis64()`, and elapsed timer helpers that avoid the ~70 minute wrap of 32-bit timers.

## Features
- 64-bit micros/millis compatible with Arduino API.
- `elapsedMicros64`, `elapsedMillis64`, `elapsedSeconds64` helper classes for simple interval logic.
- Arduino metadata (`library.properties`) and PlatformIO metadata (`library.json`).
- Examples for both Arduino IDE and PlatformIO.

## Getting started (PlatformIO)
- `cd systemchrono`
- Build the example for an ESP32 devkit: `pio run`.
- Upload + monitor: `pio run --target upload --target monitor`.
- The PIO example uses `src_dir = examples/BasicPIO/src` so the library under `src/` stays untouched.

## Getting started (Arduino IDE)
- Copy `systemchrono` into your Arduino `libraries` folder or import as ZIP.
- Include the header: `#include "SystemChrono.h"`.
- Open the example: `File -> Examples -> SystemChrono -> BasicArduino`.

## Usage
```cpp
#include <Arduino.h>
#include "SystemChrono.h"
using namespace SystemChrono;

elapsedMillis64 heartbeat;

void loop() {
  if (heartbeat >= 1000) {
    heartbeat = 0;
    Serial.printf("millis64=%lld micros64=%lld\n", (long long)millis64(), (long long)micros64());
  }
}
```

## Files
- `src/SystemChrono.h` / `src/SystemChrono.cpp` - library API and implementation.
- `library.properties` - Arduino metadata.
- `library.json` - PlatformIO metadata.
- `platformio.ini` - builds `examples/BasicPIO` for `esp32dev`.
- `examples/BasicArduino/BasicArduino.ino` - Arduino IDE example.
- `examples/BasicPIO/src/main.cpp` - PlatformIO example.
- `LICENSE` - MIT.

## Notes
- ESP32 uses `esp_timer_get_time()` for true 64-bit monotonic microseconds.
- Other Arduino cores use wrap-tracking on `micros()`; keep ISRs short to minimize skew while reading.