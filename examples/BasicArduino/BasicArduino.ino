#include <Arduino.h>
#include "SystemChrono.h"

using namespace SystemChrono;

elapsedMillis64 heartbeat;
elapsedMicros64 block_time;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ;
  }
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("SystemChrono BasicArduino example");
}

void loop() {
  if (heartbeat >= 1000) {
    heartbeat = 0;
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    Serial.printf("millis64=%lld micros64=%lld\n", (long long)millis64(), (long long)micros64());
  }

  block_time = 0;
  delayMicroseconds(50);
  Serial.printf("Block took %lld us\n", (long long)block_time);

  delay(10);
}
