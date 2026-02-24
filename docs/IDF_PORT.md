# SystemChrono — ESP-IDF Migration Prompt

> **Library**: SystemChrono (monotonic/wall-clock time utilities)
> **Current version**: 1.1.0 → **Target**: 2.0.0
> **Namespace**: `SystemChrono`
> **Include path**: `#include "SystemChrono/SystemChrono.h"`
> **Difficulty**: Medium — Arduino.h in **public header**, String return types, `#error` guard

---

## Pre-Migration

```bash
git tag v1.1.0   # freeze Arduino-era version
```

---

## Current State — Arduino Dependencies (exact)

| API | Location | Notes |
|-----|----------|-------|
| `#include <Arduino.h>` | **Header** line 18 | Public API dependency |
| `#error` guard | Lines 12-13 | Blocks non-Arduino builds |
| `String` return types | Multiple public methods | Arduino String in API |
| `esp_timer_get_time()` | Already used on ESP32 | Conditional in .cpp |

The library already uses `esp_timer_get_time()` on ESP32 targets behind `#ifdef`. The `Config` struct is currently empty.

---

## Steps

### 1. Remove the `#error` guard (lines 12-13)

Delete the compile-time check that blocks non-Arduino platforms.

### 2. Remove `#include <Arduino.h>` from the header

This is in the **public header** (line 18), so removing it affects all consumers.

Replace with the specific ESP-IDF headers needed:

```cpp
#include <cstdint>
#include <cstring>
#include "esp_timer.h"
```

### 3. Remove all `String` return types from public API

Replace every method that returns `Arduino String` with one of:
- `const char*` (for static/pooled strings)
- `size_t format(char* buf, size_t bufLen)` pattern (for formatted output)

For example, if there's a method like:

```cpp
String formatUptime();  // old
```

Replace with:

```cpp
size_t formatUptime(char* buf, size_t bufLen);  // new — returns chars written
```

This is a breaking API change — acceptable for v2.0.0.

### 4. Remove conditional Arduino/ESP-IDF `#ifdef` paths

Since we're ESP-IDF only, remove all `#ifdef ARDUINO` branches. Keep only the `esp_timer_get_time()` code paths.

### 5. Add `CMakeLists.txt` (library root)

```cmake
idf_component_register(
    SRCS "src/SystemChrono.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_timer
)
```

### 6. Add `idf_component.yml` (library root)

```yaml
version: "2.0.0"
description: "Monotonic and wall-clock time utilities for ESP-IDF"
targets:
  - esp32s2
  - esp32s3
dependencies:
  idf: ">=5.0"
```

### 7. Version bump

- `library.json` → `2.0.0`
- `Version.h` (if present) → `2.0.0`

### 8. Replace Arduino example with ESP-IDF example

Create `examples/espidf_basic/main/main.cpp`:

```cpp
#include <cstdio>
#include "SystemChrono/SystemChrono.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void app_main() {
    SystemChrono::Config cfg{};
    SystemChrono::Clock clock;
    clock.begin(cfg);

    while (true) {
        uint32_t now = clock.uptimeMs();
        char buf[64];
        clock.formatUptime(buf, sizeof(buf));
        printf("Uptime: %s (%u ms)\n", buf, now);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

Create `examples/espidf_basic/main/CMakeLists.txt`:

```cmake
idf_component_register(SRCS "main.cpp" INCLUDE_DIRS "." REQUIRES esp_timer)
```

Create `examples/espidf_basic/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.16)
set(EXTRA_COMPONENT_DIRS "../..")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(systemchrono_espidf_basic)
```

---

## Verification

```bash
cd examples/espidf_basic && idf.py set-target esp32s2 && idf.py build
```

- [ ] `idf.py build` succeeds
- [ ] Zero `#include <Arduino.h>` anywhere (including headers!)
- [ ] Zero `String` types in public API
- [ ] No `#error` blocking non-Arduino
- [ ] No `#ifdef ARDUINO` remaining
- [ ] Version bumped to 2.0.0
- [ ] `git tag v2.0.0`
