#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

REQUIRED_COMPONENTS = ["SystemChrono", "esp_timer", "esp_rom", "freertos", "vfs"]
REQUIRED_FILES = [
    "CMakeLists.txt",
    "idf_component.yml",
    "examples/espidf_basic/CMakeLists.txt",
    "examples/espidf_basic/main/CMakeLists.txt",
    "examples/espidf_basic/main/main.cpp",
]
REQUIRED_NATIVE_TOKENS = [
    'extern "C" void app_main(void)',
    "fcntl",
    "STDIN_FILENO",
    "::read",
    "vTaskDelay",
    "esp_rom_delay_us",
]
FORBIDDEN_IDF_TOKENS = [
    "Arduino.h",
    "IdfArduinoCompat",
    "Serial",
    "millis()",
    "delay(",
    "#include \"examples/01_basic_bringup_cli/main.cpp\"",
]
MANDATORY_COMMANDS = [
    "help",
    "version",
    "info",
    "status",
    "config",
    "time",
    "uptime",
    "format",
    "stamp",
    "since",
    "measure",
    "start",
    "stop",
    "resume",
    "reset",
    "elapsed",
]


def fail(msg: str) -> None:
    print(f"IDF example contract FAILED: {msg}")
    raise SystemExit(1)


def require_token(text: str, token: str, label: str) -> None:
    if token not in text:
        fail(f"{label} missing token '{token}'")


def main() -> int:
    for rel in REQUIRED_FILES:
        if not (ROOT / rel).exists():
            fail(f"missing {rel}")

    idf_main = (ROOT / "examples" / "espidf_basic" / "main" / "main.cpp").read_text(
        encoding="utf-8", errors="replace"
    )
    for token in REQUIRED_NATIVE_TOKENS:
        require_token(idf_main, token, "ESP-IDF main")
    for token in FORBIDDEN_IDF_TOKENS:
        if token in idf_main:
            fail(f"ESP-IDF main must not use Arduino compatibility token '{token}'")

    cmake = (ROOT / "examples" / "espidf_basic" / "main" / "CMakeLists.txt").read_text(
        encoding="utf-8", errors="replace"
    )
    for component in REQUIRED_COMPONENTS:
        if re.search(rf"\b{re.escape(component)}\b", cmake) is None:
            fail(f"ESP-IDF CMake missing required component '{component}'")

    for command in MANDATORY_COMMANDS:
        if f'printHelpItem("{command}' not in idf_main:
            fail(f"CLI missing help item '{command}'")
        if f'strcmp(line, "{command}") == 0' not in idf_main:
            fail(f"CLI missing dispatch '{command}'")

    manifest = (ROOT / "idf_component.yml").read_text(encoding="utf-8", errors="replace")
    for token in ("esp32s2", "esp32s3", 'idf: ">=6.0.1"'):
        require_token(manifest, token, "idf_component.yml")

    print("IDF example contract PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
