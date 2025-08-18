# List of source file paths for static code analysis
from pathlib import Path

Embedded_System_Dir = (
    "C:\Workspace_Personal\ESP32\Embedded_IoT_BT_WIFI_Base_Project\embedded_system"
)


source_files = [
    "{Embedded_System_Dir}/Source/Scripts/static_code_analysis/test.cpp",
]

# Header files
header_files = []

# All files for analysis
all_files = source_files + header_files
