@echo off
chcp 65001 >nul
echo Please select build environment:
echo 1. pico32
echo 2. esp32-s3-devkitc-1
echo.
set /p choice=Please enter option (1-2): 

if "%choice%"=="1" (
    set ENV=pico32
) else if "%choice%"=="2" (
    set ENV=esp32-s3-devkitc-1
) else (
    echo Invalid option, using default environment pico32
    set ENV=pico32
)

echo.
echo Build environment: %ENV%
echo.

cd /d "%~dp0..\"
pio run -e %ENV%
pause