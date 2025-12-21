@echo off
chcp 65001 >nul
echo 请选择编译环境：
echo 1. pico32
echo 2. esp32-s3-devkitc-1
echo.
set /p choice=请输入选项 (1-2): 

if "%choice%"=="1" (
    set ENV=pico32
) else if "%choice%"=="2" (
    set ENV=esp32-s3-devkitc-1
) else (
    echo 无效选择，使用默认环境 pico32
    set ENV=pico32
)

echo.
echo 编译环境: %ENV%
echo.

cd /d "%~dp0..\"
pio run -e %ENV%
pause