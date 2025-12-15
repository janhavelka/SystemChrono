#include <Arduino.h>
#include "SystemChrono.h"

using namespace SystemChrono;

static elapsedMillis64 heartbeat_ms(0);
static elapsedMicros64 measurement_us(0);
static Stopwatch stopwatch;

static void handleSerialCommand(char cmd) {
  switch (cmd) {
    case 's': // start (reset + run)
      stopwatch.start();
      Serial.println("Stopwatch started");
      break;
    case 'p': // pause/stop
      stopwatch.stop();
      Serial.println("Stopwatch stopped");
      break;
    case 'r': // resume without clearing
      stopwatch.resume();
      Serial.println("Stopwatch resumed");
      break;
    case 'c': // clear but keep state
      stopwatch.reset();
      Serial.println("Stopwatch reset");
      break;
    case 't': // print elapsed
      Serial.printf("Stopwatch elapsed: %lld ms (%s)\n",
                    (long long)stopwatch.elapsedMillis(),
                    formatTime(stopwatch.elapsedMicros()).c_str());
      break;
    case 'h':
      Serial.println("Commands: s=start, p=stop, r=resume, c=reset, t=print, h=help");
      break;
    default:
      Serial.printf("Unknown command '%c'. Send 'h' for help.\n", cmd);
      break;
  }
}

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

  while (Serial.available() > 0) {
    char cmd = (char)Serial.read();
    // Ignore line endings
    if (cmd == '\n' || cmd == '\r') {
      continue;
    }
    handleSerialCommand(cmd);
  }

  delay(10);
}
