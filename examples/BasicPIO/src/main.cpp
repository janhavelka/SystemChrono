#include <Arduino.h>
#include "SystemChrono.h"

using namespace SystemChrono;

static elapsedMillis64 heartbeat_ms(0);
static elapsedMicros64 measurement_us(0);
static Stopwatch stopwatch;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ;
  }
  //pinMode(LED_BUILTIN, OUTPUT);
  measurement_us = 0;
  stopwatch.start();
  Serial.println("SystemChrono BasicPIO example");
}

void loop() {
  if (heartbeat_ms >= 1000) {
    heartbeat_ms = 0;
    //digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

    measurement_us = 0;
    delayMicroseconds(50);
    int64_t elapsed = measurement_us;

    Serial.printf("millis64: %lld, micros64: %lld, human: %s, block: %lld us, stopwatch: %lld ms (%s)\n",
                  (long long)millis64(),
                  (long long)micros64(),
                  formatNow().c_str(),
                  (long long)elapsed,
                  (long long)stopwatch.elapsedMillis(),
                  formatTime(stopwatch.elapsedMicros()).c_str());
  }

  delay(10);
}
