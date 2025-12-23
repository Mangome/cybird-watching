@echo off
chcp 65001 >nul

echo Please select build environment:
echo 1. pico32
echo 2. esp32-s3-devkitc-1
echo 3. esp32-s3-debug
echo.
set /p choice=Please enter option (1-3): 

if "%choice%"=="1" (
    set ENV=pico32
    set RUN_MONITOR=1
) else if "%choice%"=="2" (
    set ENV=esp32-s3-devkitc-1
    set RUN_MONITOR=0
) else if "%choice%"=="3" (
    set ENV=esp32-s3-debug
    set RUN_MONITOR=1
) else (
    echo Invalid option, using default environment pico32
    set ENV=pico32
    set RUN_MONITOR=1
)

echo.
echo Build environment: %ENV%

REM Read platformio.ini configuration (pass platform parameter)
call "%~dp0read_platformio.bat" %ENV%
if "%COM_PORT%"=="" set "COM_PORT=COM8"
if "%BAUD_RATE%"=="" set "BAUD_RATE=115200"

echo Serial port: %COM_PORT%
echo Baud rate: %BAUD_RATE%
echo.

cd /d "%~dp0..\"
if "%RUN_MONITOR%"=="1" (
    pio run -e %ENV% --target upload && pio device monitor -e %ENV%
) else (
    pio run -e %ENV% --target upload
)