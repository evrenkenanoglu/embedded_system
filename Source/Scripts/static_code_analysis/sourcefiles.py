# List of source file paths for static code analysis
Embedded_System_Dir = (
    "C:/Workspace_Personal/ESP32/Embedded_IoT_BT_WIFI_Base_Project/embedded_system"
)


source_files = [
    # f'{Embedded_System_Dir}/Source/Scripts/static_code_analysis/testfiles/test.cpp'
    # f'{Embedded_System_Dir}/Source/Scripts/static_code_analysis/testfiles/test_cert.cpp'
    # f'{Embedded_System_Dir}/Source/Scripts/static_code_analysis/testfiles/test_misra.cpp'
    f'{Embedded_System_Dir}/Source/Scripts/static_code_analysis/testfiles/test_iot_security.cpp'
    # f'{Embedded_System_Dir}/Source/System/error_macros.h'
]

# Header files
header_files = []

# All files for analysis
all_files = source_files + header_files
