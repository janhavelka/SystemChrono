/**
 * @file main.cpp
 * @brief Native ESP-IDF entry point for the SystemChrono bring-up CLI.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <esp_rom_sys.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "SystemChrono/SystemChrono.h"
#include "SystemChrono/Version.h"

namespace {

constexpr char LOG_COLOR_RESET[] = "\033[0m";
constexpr char LOG_COLOR_RED[] = "\033[31m";
constexpr char LOG_COLOR_GREEN[] = "\033[32m";
constexpr char LOG_COLOR_YELLOW[] = "\033[33m";
constexpr char LOG_COLOR_CYAN[] = "\033[36m";
constexpr size_t LINE_CAPACITY = 64U;

SystemChrono::ElapsedMillis64 g_heartbeat(0);
SystemChrono::ElapsedMicros64 g_measurement(0);
SystemChrono::ElapsedSeconds64 g_uptime;
SystemChrono::Stopwatch g_stopwatch;

int64_t g_stampUs = 0;
int64_t g_stampMs = 0;
int64_t g_stampS = 0;
bool g_hasStamp = false;
char g_line[LINE_CAPACITY] = {};
size_t g_lineLen = 0;

TickType_t delayTicks(uint32_t ms) {
  TickType_t ticks = pdMS_TO_TICKS(ms);
  if (ticks == 0 && ms > 0U) {
    ticks = 1;
  }
  return ticks;
}

void configureConsole() {
  setvbuf(stdout, nullptr, _IONBF, 0);
  const int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  if (flags >= 0) {
    (void)fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
  }
}

char* readLine() {
  while (true) {
    uint8_t value = 0;
    const ssize_t readCount = ::read(STDIN_FILENO, &value, 1U);
    if (readCount != 1) {
      return nullptr;
    }
    const char c = static_cast<char>(value);
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      g_line[g_lineLen] = '\0';
      g_lineLen = 0;
      return g_line;
    }
    if (g_lineLen + 1U < sizeof(g_line)) {
      g_line[g_lineLen++] = c;
    }
  }
}

void printVersion() {
  printf("Version: %s\nBuilt:   %s\nCommit:  %s (%s)\n",
         SystemChrono::VERSION,
         SystemChrono::BUILD_TIMESTAMP,
         SystemChrono::GIT_COMMIT,
         SystemChrono::GIT_STATUS);
}

void printHelpItem(const char* command, const char* desc) {
  printf("  %s%-16s%s - %s\n", LOG_COLOR_CYAN, command, LOG_COLOR_RESET, desc);
}

void printHelp() {
  printf("\n%s=== SystemChrono Native ESP-IDF CLI ===%s\n", LOG_COLOR_CYAN, LOG_COLOR_RESET);
  printVersion();
  printf("\n%s[Common]%s\n", LOG_COLOR_GREEN, LOG_COLOR_RESET);
  printHelpItem("help", "Show this help");
  printHelpItem("version", "Print build/version metadata");
  printHelpItem("info", "Print time source and API diagnostics");
  printHelpItem("status", "Print current timer state");
  printHelpItem("config", "Print static configuration notes");
  printf("\n%s[Time]%s\n", LOG_COLOR_GREEN, LOG_COLOR_RESET);
  printHelpItem("time", "Show current 64-bit time values");
  printHelpItem("uptime", "Show uptime");
  printHelpItem("format", "Show human-readable time");
  printHelpItem("stamp", "Capture a timestamp");
  printHelpItem("since", "Show elapsed since last stamp");
  printHelpItem("measure", "Measure esp_rom_delay_us(50)");
  printf("\n%s[Stopwatch]%s\n", LOG_COLOR_GREEN, LOG_COLOR_RESET);
  printHelpItem("start", "Reset and start stopwatch");
  printHelpItem("stop", "Stop stopwatch");
  printHelpItem("resume", "Resume stopwatch");
  printHelpItem("reset", "Clear stopwatch");
  printHelpItem("elapsed", "Show stopwatch elapsed time");
  printf("\n");
}

void cmdTime() {
  printf("micros64:  %lld\n", static_cast<long long>(SystemChrono::micros64()));
  printf("millis64:  %lld\n", static_cast<long long>(SystemChrono::millis64()));
  printf("seconds64: %lld\n", static_cast<long long>(SystemChrono::seconds64()));
}

void cmdConfig() {
  printf("Config: no runtime configuration required\n");
  printf("TIME_FORMAT_BUFFER_SIZE=%lu\n", static_cast<unsigned long>(SystemChrono::TIME_FORMAT_BUFFER_SIZE));
  printf("Time source: esp_timer_get_time\n");
}

void cmdInfo() {
  printVersion();
  cmdConfig();
  printf("Ownership: no pins, buses, tasks, or storage\n");
  printf("Allocation-free APIs: micros64/millis64/seconds64, elapsed timers, format*To\n");
}

void cmdStatus() {
  char timeBuf[SystemChrono::TIME_FORMAT_BUFFER_SIZE] = {};
  const SystemChrono::Status status = SystemChrono::formatNowTo(timeBuf, sizeof(timeBuf));
  if (status.ok()) {
    printf("Now: %s\n", timeBuf);
  } else {
    printf("%sERR%s formatNowTo: %s\n", LOG_COLOR_RED, LOG_COLOR_RESET, status.msg);
  }
  const bool running = g_stopwatch.isRunning();
  printf("Stopwatch: %s%s%s %lld ms\n",
         running ? LOG_COLOR_GREEN : LOG_COLOR_YELLOW,
         running ? "running" : "stopped",
         LOG_COLOR_RESET,
         static_cast<long long>(g_stopwatch.elapsedMillis()));
  printf("Stamp captured: %s\n", g_hasStamp ? "true" : "false");
}

void cmdFormat() {
  char timeBuf[SystemChrono::TIME_FORMAT_BUFFER_SIZE] = {};
  const SystemChrono::Status status = SystemChrono::formatNowTo(timeBuf, sizeof(timeBuf));
  if (status.ok()) {
    printf("Current time: %s\n", timeBuf);
  } else {
    printf("%sERR%s formatNowTo: %s\n", LOG_COLOR_RED, LOG_COLOR_RESET, status.msg);
  }
}

void cmdUptime() {
  const int64_t secs = static_cast<int64_t>(g_uptime);
  char formatted[SystemChrono::TIME_FORMAT_BUFFER_SIZE] = {};
  const SystemChrono::Status status = SystemChrono::formatTimeTo(SystemChrono::micros64(), formatted, sizeof(formatted));
  if (!status.ok()) {
    printf("%sERR%s formatTimeTo: %s\n", LOG_COLOR_RED, LOG_COLOR_RESET, status.msg);
    return;
  }
  printf("Uptime: %lld s (%lld:%02lld:%02lld) formatted=%s\n",
         static_cast<long long>(secs),
         static_cast<long long>(secs / 3600),
         static_cast<long long>((secs % 3600) / 60),
         static_cast<long long>(secs % 60),
         formatted);
}

void cmdStamp() {
  g_stampUs = SystemChrono::micros64();
  g_stampMs = SystemChrono::millis64();
  g_stampS = SystemChrono::seconds64();
  g_hasStamp = true;
  char timeBuf[SystemChrono::TIME_FORMAT_BUFFER_SIZE] = {};
  (void)SystemChrono::formatTimeTo(g_stampUs, timeBuf, sizeof(timeBuf));
  printf("Timestamp captured at %s\n", timeBuf);
  printf("  micros64 = %lld\n  millis64 = %lld\n  seconds64= %lld\n",
         static_cast<long long>(g_stampUs),
         static_cast<long long>(g_stampMs),
         static_cast<long long>(g_stampS));
}

void cmdSince() {
  if (!g_hasStamp) {
    printf("%sWARN%s no timestamp captured; use stamp first\n", LOG_COLOR_YELLOW, LOG_COLOR_RESET);
    return;
  }
  const int64_t elUs = SystemChrono::microsSince(g_stampUs);
  char elapsed[SystemChrono::TIME_FORMAT_BUFFER_SIZE] = {};
  (void)SystemChrono::formatTimeTo(elUs, elapsed, sizeof(elapsed));
  printf("Elapsed since stamp: %s\n", elapsed);
  printf("  microsSince  = %lld us\n", static_cast<long long>(elUs));
  printf("  millisSince  = %lld ms\n", static_cast<long long>(SystemChrono::millisSince(g_stampMs)));
  printf("  secondsSince = %lld s\n", static_cast<long long>(SystemChrono::secondsSince(g_stampS)));
}

void cmdMeasure() {
  g_measurement = 0;
  esp_rom_delay_us(50);
  printf("esp_rom_delay_us(50) took %lld us\n", static_cast<long long>(static_cast<int64_t>(g_measurement)));
}

void cmdElapsed() {
  char elapsed[SystemChrono::TIME_FORMAT_BUFFER_SIZE] = {};
  const SystemChrono::Status status = SystemChrono::formatTimeTo(g_stopwatch.elapsedMicros(), elapsed, sizeof(elapsed));
  if (!status.ok()) {
    printf("%sERR%s formatTimeTo: %s\n", LOG_COLOR_RED, LOG_COLOR_RESET, status.msg);
    return;
  }
  printf("Stopwatch: %lld ms (%s) [%s]\n",
         static_cast<long long>(g_stopwatch.elapsedMillis()),
         elapsed,
         g_stopwatch.isRunning() ? "running" : "stopped");
}

void processCommand(const char* line) {
  if (line == nullptr || line[0] == '\0') {
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
  } else if (strcmp(line, "measure") == 0) {
    cmdMeasure();
  } else if (strcmp(line, "start") == 0) {
    g_stopwatch.start();
    printf("Stopwatch started\n");
  } else if (strcmp(line, "stop") == 0) {
    g_stopwatch.stop();
    printf("Stopwatch stopped\n");
  } else if (strcmp(line, "resume") == 0) {
    g_stopwatch.resume();
    printf("Stopwatch resumed\n");
  } else if (strcmp(line, "reset") == 0) {
    g_stopwatch.reset();
    printf("Stopwatch reset\n");
  } else if (strcmp(line, "elapsed") == 0) {
    cmdElapsed();
  } else {
    printf("%sERR%s unknown command '%s'; type help\n", LOG_COLOR_RED, LOG_COLOR_RESET, line);
  }
}

}  // namespace

extern "C" void app_main(void) {
  configureConsole();
  g_stopwatch.start();
  printHelp();
  printf("Ready. Type a command:\n");

  while (true) {
    if (g_heartbeat >= 5000) {
      g_heartbeat = 0;
      char uptime[SystemChrono::TIME_FORMAT_BUFFER_SIZE] = {};
      const SystemChrono::Status status = SystemChrono::formatNowTo(uptime, sizeof(uptime));
      if (status.ok()) {
        printf("Uptime: %s (%llds) | Stopwatch: %lld ms [%s]\n",
               uptime,
               static_cast<long long>(static_cast<int64_t>(g_uptime)),
               static_cast<long long>(g_stopwatch.elapsedMillis()),
               g_stopwatch.isRunning() ? "running" : "stopped");
      }
    }

    char* line = readLine();
    if (line != nullptr) {
      processCommand(line);
    }
    vTaskDelay(delayTicks(1U));
  }
}
