@echo off
chcp 65001 >nul
title CybirdWatching CLI

echo ======================================
echo   CybirdWatching CLI Quick Launcher
echo ======================================
echo.

REM Select platform
echo Please select target platform:
echo 1. pico32 (ESP32)
echo 2. esp32-s3-devkitc-1 (ESP32-S3 Release)
echo 3. esp32-s3-debug (ESP32-S3 Debug)
echo.
set /p choice=Please enter option (1-3): 

if "%choice%"=="1" (
    set PLATFORM=pico32
) else if "%choice%"=="2" (
    set PLATFORM=esp32-s3-devkitc-1
) else if "%choice%"=="3" (
    set PLATFORM=esp32-s3-debug
) else (
    echo Invalid option, using default platform pico32
    set PLATFORM=pico32
)

echo.
echo Target platform: %PLATFORM%

REM Read platformio.ini configuration (pass platform parameter)
call "%~dp0read_platformio.bat" %PLATFORM%
if "%COM_PORT%"=="" (
    echo Warning: Cannot read configuration, using default port COM3
    set "COM_PORT=COM3"
)

echo Using port: %COM_PORT%
echo.

REM Check if in correct directory
if not exist "cybird_watching_cli\src\cybird_watching_cli\main.py" (
    echo Error: Cannot find cybird_watching_cli directory
    echo Please ensure this bat file is located in the scripts directory
    pause
    exit /b 1
)

echo Starting CybirdWatching CLI interactive mode...
echo.

REM Enter CLI directory and start
cd /d cybird_watching_cli

REM Check if uv is installed
where uv >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: uv package manager not found
    echo Please install uv first: https://docs.astral.sh/uv/
    pause
    exit /b 1
)

REM Start CLI (pass platform information)
echo.
echo Launch parameters: 
echo   - Serial port: %COM_PORT%
echo   - Platform: %PLATFORM%
echo.
uv run python -m cybird_watching_cli.main -p %COM_PORT% --platform %PLATFORM%

echo.
echo Thank you for using CybirdWatching CLI!
pause