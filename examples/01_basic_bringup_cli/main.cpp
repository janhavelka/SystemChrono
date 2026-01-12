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
  while (LOG_SERIAL.available()) {
    const char c = static_cast<char>(LOG_SERIAL.read());
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
  LOG_SERIAL.println();
  LOG_SERIAL.println(F("=== SystemChrono CLI ==="));
  LOG_SERIAL.print(F("Version: "));
  LOG_SERIAL.println(SystemChrono::VERSION);
  LOG_SERIAL.print(F("Built:   "));
  LOG_SERIAL.println(SystemChrono::BUILD_TIMESTAMP);
  LOG_SERIAL.print(F("Commit:  "));
  LOG_SERIAL.print(SystemChrono::GIT_COMMIT);
  LOG_SERIAL.print(F(" ("));
  LOG_SERIAL.print(SystemChrono::GIT_STATUS);
  LOG_SERIAL.println(F(")"));
  LOG_SERIAL.println();
  LOG_SERIAL.println(F("Time Commands:"));
  LOG_SERIAL.println(F("  time         - Show current 64-bit time values"));
  LOG_SERIAL.println(F("  format       - Show human-readable time (HH:MM:SS.mmm)"));
  LOG_SERIAL.println();
  LOG_SERIAL.println(F("Stopwatch Commands:"));
  LOG_SERIAL.println(F("  start        - Reset and start stopwatch"));
  LOG_SERIAL.println(F("  stop         - Stop stopwatch"));
  LOG_SERIAL.println(F("  resume       - Resume stopwatch"));
  LOG_SERIAL.println(F("  reset        - Clear stopwatch"));
  LOG_SERIAL.println(F("  elapsed      - Show stopwatch elapsed time"));
  LOG_SERIAL.println();
  LOG_SERIAL.println(F("Other:"));
  LOG_SERIAL.println(F("  measure      - Measure delayMicroseconds(50) overhead"));
  LOG_SERIAL.println(F("  help         - Show this help"));
  LOG_SERIAL.println();
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
  char buf[SystemChrono::kFormatTimeBufferSize];
  const Status st = formatNowToBuffer(buf, sizeof(buf));
  if (!st.ok()) {
    LOGE("formatNowToBuffer failed: %s", st.msg);
    return;
  }
  LOGI("Current time: %s", buf);
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
  char buf[SystemChrono::kFormatTimeBufferSize];
  const Status st = formatTimeToBuffer(g_stopwatch.elapsedMicros(), buf, sizeof(buf));
  if (!st.ok()) {
    LOGE("formatTimeToBuffer failed: %s", st.msg);
    return;
  }
  LOGI("Stopwatch: %lld ms (%s) [%s]",
       static_cast<long long>(g_stopwatch.elapsedMillis()),
       buf,
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

  const Config config;
  const Status st = SystemChrono::begin(config);
  if (!st.ok()) {
    LOGE("SystemChrono begin failed: %s", st.msg);
  }

  // Initialize stopwatch
  g_stopwatch.start();

  printHelp();
  LOG_SERIAL.println(F("Ready. Type a command:"));
}

void loop() {
  SystemChrono::tick(static_cast<uint32_t>(millis()));

  // Periodic heartbeat output (every 5 seconds)
  if (g_heartbeat >= 5000) {
    g_heartbeat = 0;
    char buf[SystemChrono::kFormatTimeBufferSize];
    const Status st = formatNowToBuffer(buf, sizeof(buf));
    if (st.ok()) {
      LOGI("Uptime: %s | Stopwatch: %lld ms [%s]",
           buf,
           static_cast<long long>(g_stopwatch.elapsedMillis()),
           g_stopwatch.isRunning() ? "running" : "stopped");
    } else {
      LOGI("Stopwatch: %lld ms [%s]",
           static_cast<long long>(g_stopwatch.elapsedMillis()),
           g_stopwatch.isRunning() ? "running" : "stopped");
    }
  }

  // Non-blocking command processing
  const String line = readLine();
  if (line.length() > 0) {
    processCommand(line);
  }
}
