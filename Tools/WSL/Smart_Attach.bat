@echo off
setlocal

:: --- CONFIGURATION ---
:: Silicon Labs CP210x VID:PID
set "TARGET_VID_PID=10c4:ea60"
:: ---------------------

:: 1. Self-elevate to Admin if not already
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo Requesting Admin privileges...
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

echo Searching for ESP32 (ID: %TARGET_VID_PID%)...

:: 2. Find the BUSID dynamically
set "BUSID="
for /f "tokens=1" %%a in ('usbipd list ^| findstr "%TARGET_VID_PID%"') do (
    set "BUSID=%%a"
)

:: 3. Check if found
if "%BUSID%"=="" (
    echo.
    echo [ERROR] ESP32 not found! 
    echo Please make sure the device is plugged in.
    echo.
    pause
    exit /b
)

:: 4. Attach to WSL
echo Found ESP32 at Bus ID: %BUSID%
echo Attaching to WSL...

:: Bind is persistent, so we run it just in case it's new
usbipd bind --busid %BUSID% >nul 2>&1

:: Attach with auto-connect
usbipd attach --wsl --busid %BUSID% --auto-attach

:: 5. Verification and Pause
if %errorLevel% equ 0 (
    echo.
    echo [SUCCESS] ESP32 is now attached to WSL!
    echo.
    echo --- Current Device Status ---
    usbipd list | findstr "%TARGET_VID_PID%"
    echo.
    echo Press any key to close this window...
    pause >nul
) else (
    echo.
    echo [ERROR] Failed to attach. 
    echo The device might be in use or busy.
    echo.
    pause
)

endlocal