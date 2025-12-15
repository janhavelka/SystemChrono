#include <Arduino.h>
#include "SystemChrono64.h"

using namespace SystemChrono64;

static elapsedMillis64 heartbeat_ms(0);
static elapsedMicros64 measurement_us(0);

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ;
  }
  pinMode(LED_BUILTIN, OUTPUT);
  measurement_us = 0;
  Serial.println("SystemChrono64 BasicPIO example");
}

void loop() {
  if (heartbeat_ms >= 1000) {
    heartbeat_ms = 0;
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    Serial.printf("millis64: %lld, micros64: %lld\n", (long long)millis64(), (long long)micros64());
  }

  measurement_us = 0;
  delayMicroseconds(50);
  int64_t elapsed = measurement_us;
  if (elapsed > 0) {
    Serial.printf("Measured %lld us\n", (long long)elapsed);
  }

  delay(10);
}
