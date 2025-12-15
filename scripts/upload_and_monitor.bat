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

REM 读取platformio.ini配置
call "%~dp0read_platformio.bat"
if "%COM_PORT%"=="" set "COM_PORT=COM8"
if "%BAUD_RATE%"=="" set "BAUD_RATE=115200"

echo.
echo 编译环境: %ENV%
echo 串口: %COM_PORT%
echo 波特率: %BAUD_RATE%
echo.

cd /d "%~dp0..\"
pio run -e %ENV% --target upload && pio device monitor --port %COM_PORT% --baud %BAUD_RATE%