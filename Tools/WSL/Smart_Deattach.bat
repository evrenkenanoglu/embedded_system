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
echo Detaching from WSL...

usbipd bind --busid %BUSID%
usbipd detach --busid %BUSID%

if %errorLevel% equ 0 (
    echo.
    echo [SUCCESS] ESP32 is now attached to WSL!
    echo You can close this window.
    timeout /t 5 >nul
) else (
    echo.
    echo [ERROR] Failed to attach. 
    pause
)

:: 5. Print list 
echo.
echo Current USB devices attached to WSL:
usbipd list --wsl
endlocal