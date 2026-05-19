/**
 * @file main.cpp
 * @brief Minimal ESP-IDF SystemChrono example.
 */

#include <stdint.h>

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "SystemChrono/SystemChrono.h"

namespace {

static constexpr char TAG[] = "systemchrono_idf";
static constexpr uint32_t LOOP_DELAY_MS = 1000U;

}  // namespace

extern "C" void app_main(void) {
  SystemChrono::ElapsedMillis64 heartbeat;
  SystemChrono::Stopwatch stopwatch;
  stopwatch.start();

  while (true) {
    char formatted[SystemChrono::TIME_FORMAT_BUFFER_SIZE] = {};
    const SystemChrono::Status status =
        SystemChrono::formatNowTo(formatted, sizeof(formatted));
    if (status.ok()) {
      ESP_LOGI(TAG,
               "now=%s micros=%lld millis=%lld seconds=%lld heartbeat=%lld stopwatch=%lld",
               formatted,
               static_cast<long long>(SystemChrono::micros64()),
               static_cast<long long>(SystemChrono::millis64()),
               static_cast<long long>(SystemChrono::seconds64()),
               static_cast<long long>(heartbeat),
               static_cast<long long>(stopwatch.elapsedMillis()));
    } else {
      ESP_LOGE(TAG, "formatNowTo failed: %s (%d, %ld)", status.msg,
               static_cast<int>(status.code), static_cast<long>(status.detail));
    }

    heartbeat = 0;
    vTaskDelay(pdMS_TO_TICKS(LOOP_DELAY_MS));
  }
}
