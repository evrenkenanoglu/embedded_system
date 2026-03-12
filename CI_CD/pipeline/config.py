import os
import platform

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
EXTRACTED_BUILD_DIR = os.path.join(PROJECT_ROOT, "build_extracted")
HIL_TEST_DIR = os.path.join(PROJECT_ROOT, "tests", "hil")

# --- PLATFORM / TOOLCHAIN SELECTION ---
TOOLCHAIN = "esp-idf"          # In the future, change to "zephyr", "stm32", etc.
DEFAULT_TARGET = "esp32"       # E.g., "esp32", "nrf52840", "stm32f4"

# --- ESP-IDF SPECIFIC (Ignored by other toolchains) ---
IDF_VERSION = "5.2"
FLASH_ARGS_FILE = "flash_project_args"

# --- PIPELINE SETTINGS ---
ARTIFACT_NAME = "firmware_artifact" 
DEFAULT_OTA_PORT = 8032
REPORT_NAME = "hil_report.html"
DEFAULT_PORT = "COM3" if platform.system().lower() == "windows" else "/dev/ttyUSB0"