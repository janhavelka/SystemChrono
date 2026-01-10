/**
 * @file main.cpp
 * @brief Interactive CLI example for SystemChrono.
 *
 * Demonstrates all SystemChrono features with serial commands:
 * - 64-bit time accessors (micros64, millis64, seconds64)
 * - Elapsed timer classes (ElapsedMicros64, ElapsedMillis64, ElapsedSeconds64)
 * - Stopwatch with start/stop/resume/reset
 * - Human-readable time formatting
 *
 * Type 'help' for available commands.
 */

#include <Arduino.h>

#include "examples/common/BoardPins.h"
#include "examples/common/Log.h"
#include "SystemChrono/Version.h"
#include "SystemChrono/SystemChrono.h"

using namespace SystemChrono;

// Global timers and stopwatch
static ElapsedMillis64 g_heartbeat(0);
static ElapsedMicros64 g_measurement(0);
static Stopwatch g_stopwatch;

/**
 * @brief Non-blocking line reader from Serial.
 * @return Complete line (without newline) or empty string if incomplete.
 */
static String readLine() {
  static String buffer;
  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      String result = buffer;
      buffer = "";
      return result;
    }
    if (buffer.length() < 64) {  // Limit buffer size
      buffer += c;
    }
  }
  return "";
}

/**
 * @brief Print available commands.
 */
static void printHelp() {
  Serial.println();
  Serial.println(F("=== SystemChrono CLI ==="));
  Serial.print(F("Version: "));
  Serial.println(SystemChrono::VERSION);
  Serial.print(F("Built:   "));
  Serial.println(SystemChrono::BUILD_TIMESTAMP);
  Serial.print(F("Commit:  "));
  Serial.print(SystemChrono::GIT_COMMIT);
  Serial.print(F(" ("));
  Serial.print(SystemChrono::GIT_STATUS);
  Serial.println(F(")"));
  Serial.println();
  Serial.println(F("Time Commands:"));
  Serial.println(F("  time         - Show current 64-bit time values"));
  Serial.println(F("  format       - Show human-readable time (HH:MM:SS.mmm)"));
  Serial.println();
  Serial.println(F("Stopwatch Commands:"));
  Serial.println(F("  start        - Reset and start stopwatch"));
  Serial.println(F("  stop         - Stop stopwatch"));
  Serial.println(F("  resume       - Resume stopwatch"));
  Serial.println(F("  reset        - Clear stopwatch"));
  Serial.println(F("  elapsed      - Show stopwatch elapsed time"));
  Serial.println();
  Serial.println(F("Other:"));
  Serial.println(F("  measure      - Measure delayMicroseconds(50) overhead"));
  Serial.println(F("  help         - Show this help"));
  Serial.println();
}

/**
 * @brief Handle 'time' command - show current time values.
 */
static void cmdTime() {
  LOGI("micros64:  %lld", static_cast<long long>(micros64()));
  LOGI("millis64:  %lld", static_cast<long long>(millis64()));
  LOGI("seconds64: %lld", static_cast<long long>(seconds64()));
}

/**
 * @brief Handle 'format' command - show formatted time.
 */
static void cmdFormat() {
  LOGI("Current time: %s", formatNow().c_str());
}

/**
 * @brief Handle 'start' command.
 */
static void cmdStart() {
  g_stopwatch.start();
  LOGI("Stopwatch started");
}

/**
 * @brief Handle 'stop' command.
 */
static void cmdStop() {
  g_stopwatch.stop();
  LOGI("Stopwatch stopped");
}

/**
 * @brief Handle 'resume' command.
 */
static void cmdResume() {
  g_stopwatch.resume();
  LOGI("Stopwatch resumed");
}

/**
 * @brief Handle 'reset' command.
 */
static void cmdReset() {
  g_stopwatch.reset();
  LOGI("Stopwatch reset");
}

/**
 * @brief Handle 'elapsed' command.
 */
static void cmdElapsed() {
  LOGI("Stopwatch: %lld ms (%s) [%s]",
       static_cast<long long>(g_stopwatch.elapsedMillis()),
       formatTime(g_stopwatch.elapsedMicros()).c_str(),
       g_stopwatch.isRunning() ? "running" : "stopped");
}

/**
 * @brief Handle 'measure' command - measure timing overhead.
 */
static void cmdMeasure() {
  g_measurement = 0;
  delayMicroseconds(50);
  int64_t elapsed = g_measurement;
  LOGI("delayMicroseconds(50) took %lld us", static_cast<long long>(elapsed));
}

/**
 * @brief Process a single command line.
 * @param line The command line to process.
 */
static void processCommand(const String& line) {
  if (line == "help") {
    printHelp();
  } else if (line == "time") {
    cmdTime();
  } else if (line == "format") {
    cmdFormat();
  } else if (line == "start") {
    cmdStart();
  } else if (line == "stop") {
    cmdStop();
  } else if (line == "resume") {
    cmdResume();
  } else if (line == "reset") {
    cmdReset();
  } else if (line == "elapsed") {
    cmdElapsed();
  } else if (line == "measure") {
    cmdMeasure();
  } else {
    LOGE("Unknown command '%s'. Type 'help' for usage.", line.c_str());
  }
}

void setup() {
  log_begin(115200);
  delay(100);  // Allow USB CDC to initialize

  // Initialize stopwatch
  g_stopwatch.start();

  printHelp();
  Serial.println(F("Ready. Type a command:"));
}

void loop() {
  // Periodic heartbeat output (every 5 seconds)
  if (g_heartbeat >= 5000) {
    g_heartbeat = 0;
    LOGI("Uptime: %s | Stopwatch: %lld ms [%s]",
         formatNow().c_str(),
         static_cast<long long>(g_stopwatch.elapsedMillis()),
         g_stopwatch.isRunning() ? "running" : "stopped");
  }

  // Non-blocking command processing
  const String line = readLine();
  if (line.length() > 0) {
    processCommand(line);
  }
}
