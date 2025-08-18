@echo off
echo ================================================================
echo ESP32 IoT Project - Comprehensive Static Code Analysis
echo Standards: MISRA C++, CERT, IoT Security Best Practices
echo ================================================================
echo.

:: Check if Python is installed
python --version >nul 2>&1
if errorlevel 1 (
    echo Error: Python is not installed or not in PATH
    echo Please install Python 3.7+ and try again
    pause
    exit /b 1
)

:: Install requirements if needed
echo [1/4] Installing Python dependencies...
pip install -r requirements.txt
if errorlevel 1 (
    echo Error: Failed to install dependencies
    pause
    exit /b 1
)

echo.
echo [2/4] Running comprehensive static analysis...
echo This may take a few minutes...
echo.

:: Run the analysis
python analyze.py
if errorlevel 1 (
    echo Error: Analysis failed
    pause
    exit /b 1
)

echo.
echo [3/4] Analysis completed successfully!
echo.

:: Check if reports exist and open them
if exist "reports\analysis-report.txt" (
    echo [4/4] Opening analysis reports...
    start "" "reports\analysis-report.txt"
    timeout /t 2 /nobreak >nul
    if exist "reports\comprehensive-analysis.json" (
        echo JSON report also available: reports\comprehensive-analysis.json
    )
) else (
    echo Warning: Report files not found in reports directory
)

echo.
echo ================================================================
echo Analysis Complete! Check the following:
echo   1. Critical ERRORS (fix immediately)
echo   2. MISRA violations (for safety compliance)
echo   3. Security issues (for IoT security)
echo   4. Code quality warnings
echo ================================================================
echo.
pause