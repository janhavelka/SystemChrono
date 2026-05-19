/**
 * @file main.cpp
 * @brief Interactive CLI example for SystemChrono.
 *
 * Demonstrates all SystemChrono features with serial commands:
 * - 64-bit time accessors (micros64, millis64, seconds64)
 * - Elapsed helpers (microsSince, millisSince, secondsSince)
 * - Elapsed timer classes (ElapsedMicros64, ElapsedMillis64, ElapsedSeconds64)
 * - Stopwatch with start/stop/resume/reset
 * - Human-readable time formatting (allocation-free and String variants)
 *
 * Type 'help' for available commands.
 */

#include <Arduino.h>
#include <string.h>

#include "examples/common/BoardPins.h"
#include "examples/common/Log.h"
#include "SystemChrono/Version.h"
#include "SystemChrono/SystemChrono.h"

using namespace SystemChrono;

// Global timers and stopwatch
static ElapsedMillis64 g_heartbeat(0);
static ElapsedMicros64 g_measurement(0);
static ElapsedSeconds64 g_uptime;
static Stopwatch g_stopwatch;

// Timestamp captured by 'stamp' command
static int64_t g_stampUs  = 0;
static int64_t g_stampMs  = 0;
static int64_t g_stampS   = 0;
static bool    g_hasStamp = false;

static char g_line[64];
static size_t g_lineLen = 0;

static void printHelpSection(const char* title) {
  Serial.printf("%s[%s]%s\n", LOG_COLOR_GREEN, title, LOG_COLOR_RESET);
}

static void printHelpItem(const char* cmd, const char* desc) {
  Serial.printf("  %s%-16s%s - %s\n", LOG_COLOR_CYAN, cmd, LOG_COLOR_RESET, desc);
}

static const char* runStateColor(bool running) {
  return running ? LOG_COLOR_GREEN : LOG_COLOR_YELLOW;
}

static const char* timeSourceName() {
#if defined(ARDUINO_ARCH_ESP32)
  return "esp_timer_get_time";
#else
  return "micros-wrap-tracker";
#endif
}

/**
 * @brief Non-blocking line reader from Serial.
 * @return Complete line (without newline), or nullptr if incomplete.
 */
static char* readLine() {
  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      g_line[g_lineLen] = '\0';
      g_lineLen = 0;
      return g_line;
    }
    if (g_lineLen + 1 < sizeof(g_line)) {
      g_line[g_lineLen++] = c;
    }
  }
  return nullptr;
}

static void printVersion() {
  Serial.print(F("Version: "));
  Serial.println(SystemChrono::VERSION);
  Serial.print(F("Built:   "));
  Serial.println(SystemChrono::BUILD_TIMESTAMP);
  Serial.print(F("Commit:  "));
  Serial.print(SystemChrono::GIT_COMMIT);
  Serial.print(F(" ("));
  Serial.print(SystemChrono::GIT_STATUS);
  Serial.println(F(")"));
}

/**
 * @brief Print available commands.
 */
static void printHelp() {
  Serial.println();
  Serial.printf("%s=== SystemChrono CLI Help ===%s\n", LOG_COLOR_CYAN, LOG_COLOR_RESET);
  printVersion();
  Serial.println();
  printHelpSection("Common");
  printHelpItem("help", "Show this help");
  printHelpItem("version", "Print build/version metadata");
  printHelpItem("info", "Print time source and API diagnostics");
  printHelpItem("status", "Print current timer state");
  printHelpItem("config", "Print static configuration notes");
  Serial.println();
  printHelpSection("Time");
  printHelpItem("time", "Show current 64-bit time values");
  printHelpItem("uptime", "Show uptime (ElapsedSeconds64)");
  printHelpItem("format", "Show human-readable time (HH:MM:SS.mmm)");
  printHelpItem("stamp", "Capture a timestamp (micros64/millis64/seconds64)");
  printHelpItem("since", "Show elapsed since last stamp");
  printHelpItem("measure", "Measure delayMicroseconds(50) overhead");
  Serial.println();
  printHelpSection("Stopwatch");
  printHelpItem("start", "Reset and start stopwatch");
  printHelpItem("stop", "Stop stopwatch");
  printHelpItem("resume", "Resume stopwatch");
  printHelpItem("reset", "Clear stopwatch");
  printHelpItem("elapsed", "Show stopwatch elapsed time");
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

static void cmdConfig() {
  Serial.println(F("Config: no runtime configuration required"));
  Serial.print(F("TIME_FORMAT_BUFFER_SIZE="));
  Serial.println(static_cast<unsigned long>(TIME_FORMAT_BUFFER_SIZE));
  Serial.print(F("Time source: "));
  Serial.println(timeSourceName());
}

static void cmdInfo() {
  printVersion();
  cmdConfig();
  Serial.println(F("Ownership: no pins, buses, tasks, or storage"));
  Serial.println(F("Allocation-free APIs: micros64/millis64/seconds64, elapsed timers, format*To"));
}

static void cmdStatus() {
  char timeBuf[TIME_FORMAT_BUFFER_SIZE];
  const Status status = formatNowTo(timeBuf, sizeof(timeBuf));
  if (status.ok()) {
    Serial.print(F("Now: "));
    Serial.println(timeBuf);
  } else {
    LOGE("formatNowTo failed: %s", status.msg);
  }

  const bool running = g_stopwatch.isRunning();
  Serial.printf("Stopwatch: %s%s%s %lld ms\n",
                runStateColor(running),
                running ? "running" : "stopped",
                LOG_COLOR_RESET,
                static_cast<long long>(g_stopwatch.elapsedMillis()));
  Serial.print(F("Stamp captured: "));
  Serial.println(g_hasStamp ? "true" : "false");
}

/**
 * @brief Handle 'format' command - show formatted time.
 */
static void cmdFormat() {
  char timeBuf[TIME_FORMAT_BUFFER_SIZE];
  const Status status = formatNowTo(timeBuf, sizeof(timeBuf));
  if (!status.ok()) {
    LOGE("formatNowTo failed: %s (code=%d, detail=%ld)",
         status.msg,
         static_cast<int>(status.code),
         static_cast<long>(status.detail));
    return;
  }
  LOGI("Current time: %s", timeBuf);
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
  char elapsedBuf[TIME_FORMAT_BUFFER_SIZE];
  const Status status = formatTimeTo(g_stopwatch.elapsedMicros(), elapsedBuf, sizeof(elapsedBuf));
  if (!status.ok()) {
    LOGE("formatTimeTo failed: %s (code=%d, detail=%ld)",
         status.msg,
         static_cast<int>(status.code),
         static_cast<long>(status.detail));
    return;
  }

  const bool running = g_stopwatch.isRunning();
  LOGI("Stopwatch: %lld ms (%s) [%s]",
       static_cast<long long>(g_stopwatch.elapsedMillis()),
       elapsedBuf,
       running ? "running" : "stopped");
  Serial.printf("  State: %s%s%s\n",
                runStateColor(running),
                running ? "running" : "stopped",
                LOG_COLOR_RESET);
}

/**
 * @brief Handle 'uptime' command - show uptime via ElapsedSeconds64.
 */
static void cmdUptime() {
  const int64_t secs = static_cast<int64_t>(g_uptime);
  const int64_t hrs  = secs / 3600;
  const int64_t mins = (secs % 3600) / 60;
  const int64_t s    = secs % 60;

  char formatted[TIME_FORMAT_BUFFER_SIZE];
  const Status status = formatTimeTo(micros64(), formatted, sizeof(formatted));
  if (!status.ok()) {
    LOGE("formatTimeTo failed: %s", status.msg);
    return;
  }
  LOGI("Uptime: %lld s (%lld:%02lld:%02lld) | formatted: %s",
       static_cast<long long>(secs),
       static_cast<long long>(hrs),
       static_cast<long long>(mins),
       static_cast<long long>(s),
       formatted);
}

/**
 * @brief Handle 'stamp' command - capture current timestamps.
 */
static void cmdStamp() {
  g_stampUs  = micros64();
  g_stampMs  = millis64();
  g_stampS   = seconds64();
  g_hasStamp = true;

  char timeBuf[TIME_FORMAT_BUFFER_SIZE];
  const Status status = formatTimeTo(g_stampUs, timeBuf, sizeof(timeBuf));
  if (!status.ok()) {
    LOGE("formatTimeTo failed: %s", status.msg);
    return;
  }
  LOGI("Timestamp captured at %s", timeBuf);
  LOGI("  micros64 = %lld", static_cast<long long>(g_stampUs));
  LOGI("  millis64 = %lld", static_cast<long long>(g_stampMs));
  LOGI("  seconds64= %lld", static_cast<long long>(g_stampS));
}

/**
 * @brief Handle 'since' command - show elapsed since last stamp.
 */
static void cmdSince() {
  if (!g_hasStamp) {
    LOGW("No timestamp captured. Use 'stamp' first.");
    return;
  }
  const int64_t elUs = microsSince(g_stampUs);
  const int64_t elMs = millisSince(g_stampMs);
  const int64_t elS  = secondsSince(g_stampS);

  char elBuf[TIME_FORMAT_BUFFER_SIZE];
  const Status status = formatTimeTo(elUs, elBuf, sizeof(elBuf));
  if (!status.ok()) {
    LOGE("formatTimeTo failed: %s", status.msg);
    return;
  }
  LOGI("Elapsed since stamp: %s", elBuf);
  LOGI("  microsSince  = %lld us", static_cast<long long>(elUs));
  LOGI("  millisSince  = %lld ms", static_cast<long long>(elMs));
  LOGI("  secondsSince = %lld s",  static_cast<long long>(elS));
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
static void processCommand(const char* line) {
  if (!line || line[0] == '\0') {
    return;
  }
  if (strcmp(line, "help") == 0) {
    printHelp();
  } else if (strcmp(line, "version") == 0) {
    printVersion();
  } else if (strcmp(line, "info") == 0) {
    cmdInfo();
  } else if (strcmp(line, "status") == 0) {
    cmdStatus();
  } else if (strcmp(line, "config") == 0) {
    cmdConfig();
  } else if (strcmp(line, "time") == 0) {
    cmdTime();
  } else if (strcmp(line, "uptime") == 0) {
    cmdUptime();
  } else if (strcmp(line, "format") == 0) {
    cmdFormat();
  } else if (strcmp(line, "stamp") == 0) {
    cmdStamp();
  } else if (strcmp(line, "since") == 0) {
    cmdSince();
  } else if (strcmp(line, "start") == 0) {
    cmdStart();
  } else if (strcmp(line, "stop") == 0) {
    cmdStop();
  } else if (strcmp(line, "resume") == 0) {
    cmdResume();
  } else if (strcmp(line, "reset") == 0) {
    cmdReset();
  } else if (strcmp(line, "elapsed") == 0) {
    cmdElapsed();
  } else if (strcmp(line, "measure") == 0) {
    cmdMeasure();
  } else {
    LOGE("Unknown command '%s'. Type 'help' for usage.", line);
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
    char uptimeBuf[TIME_FORMAT_BUFFER_SIZE];
    const Status status = formatNowTo(uptimeBuf, sizeof(uptimeBuf));
    if (!status.ok()) {
      LOGE("formatNowTo failed: %s (code=%d, detail=%ld)",
           status.msg,
           static_cast<int>(status.code),
           static_cast<long>(status.detail));
      return;
    }

    LOGI("Uptime: %s (%llds) | Stopwatch: %lld ms [%s]",
         uptimeBuf,
         static_cast<long long>(static_cast<int64_t>(g_uptime)),
         static_cast<long long>(g_stopwatch.elapsedMillis()),
         g_stopwatch.isRunning() ? "running" : "stopped");
  }

  // Non-blocking command processing
  const char* line = readLine();
  if (line) {
    processCommand(line);
  }
}
