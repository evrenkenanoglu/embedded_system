import os
import platform

class ProjectConfig:
    # Resolves to 'smart_plug_project' root
    PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    
    # --- PIPELINE LIBRARY LOCATION ---
    # Can be overridden by the CI Server via: export PIPELINE_LIB_PATH="/opt/embedded_system/CI_CD"
    _default_lib_path = os.path.abspath(os.path.join(PROJECT_ROOT, "../../embedded_system/CI_CD"))
    PIPELINE_LIB_PATH = os.getenv("PIPELINE_LIB_PATH", _default_lib_path)

    # --- PROJECT SETTINGS ---
    TOOLCHAIN = "esp-idf"
    TARGET = "esp32"
    IDF_VERSION = "5.2"
    
    BUILD_DIR = os.path.join(PROJECT_ROOT, "build")
    EXTRACTED_BUILD_DIR = os.path.join(PROJECT_ROOT, "build_extracted")
    HIL_TEST_DIR = os.path.join(PROJECT_ROOT, "example", "tests", "hil")
    
    ARTIFACT_NAME = "firmware_artifact"
    FLASH_ARGS_FILE = "flash_project_args"
    DEFAULT_OTA_PORT = 8032
    
    DEFAULT_PORT = "COM3" if platform.system().lower() == "windows" else "/dev/ttyUSB0"

    EMBEDDED_SERVICES = ["esp", "serial"]
    
    TEST_OUTPUT_DIR = os.path.join(PROJECT_ROOT, "test_reports")
    REPORT_FILE_PREFIX = "hil_report"