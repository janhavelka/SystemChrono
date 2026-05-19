#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]

IDF_EXAMPLE_MACRO = "SYSTEMCHRONO_EXAMPLE_PLATFORM_IDF"
CLI_SOURCE_INCLUDE = '#include "examples/01_basic_bringup_cli/main.cpp"'
REQUIRED_COMPONENTS = ["SystemChrono", "esp_timer", "esp_rom", "freertos", "vfs"]
REQUIRED_FILES = [
    "CMakeLists.txt",
    "idf_component.yml",
    "examples/common/IdfArduinoCompat.h",
    "examples/espidf_basic/CMakeLists.txt",
    "examples/espidf_basic/main/CMakeLists.txt",
    "examples/espidf_basic/main/main.cpp",
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
    for token in (
        f"#define {IDF_EXAMPLE_MACRO} 1",
        '#include "examples/common/IdfArduinoCompat.h"',
        CLI_SOURCE_INCLUDE,
        'extern "C" void app_main(void)',
        "setup();",
        "loop();",
    ):
        require_token(idf_main, token, "ESP-IDF main")

    cmake = (ROOT / "examples" / "espidf_basic" / "main" / "CMakeLists.txt").read_text(
        encoding="utf-8", errors="replace"
    )
    for component in REQUIRED_COMPONENTS:
        if re.search(rf"\b{re.escape(component)}\b", cmake) is None:
            fail(f"ESP-IDF CMake missing required component '{component}'")

    compat = (ROOT / "examples" / "common" / "IdfArduinoCompat.h").read_text(
        encoding="utf-8", errors="replace"
    )
    for token in ("class IdfConsole", "esp_timer_get_time", "esp_rom_delay_us", "fcntl"):
        require_token(compat, token, "IdfArduinoCompat.h")

    cli = (ROOT / "examples" / "01_basic_bringup_cli" / "main.cpp").read_text(
        encoding="utf-8", errors="replace"
    )
    require_token(cli, f"defined({IDF_EXAMPLE_MACRO})", "shared CLI")
    for command in MANDATORY_COMMANDS:
        if f'printHelpItem("{command}' not in cli:
            fail(f"CLI missing help item '{command}'")
        if f'strcmp(line, "{command}") == 0' not in cli:
            fail(f"CLI missing dispatch '{command}'")

    manifest = (ROOT / "idf_component.yml").read_text(encoding="utf-8", errors="replace")
    for token in ("esp32s2", "esp32s3", 'idf: ">=6.0.1"'):
        require_token(manifest, token, "idf_component.yml")

    print("IDF example contract PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
