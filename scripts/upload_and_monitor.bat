@echo off
chcp 65001 >nul
REM 读取platformio.ini配置
call "%~dp0read_platformio.bat"
if "%COM_PORT%"=="" set "COM_PORT=COM3"
if "%BAUD_RATE%"=="" set "BAUD_RATE=115200"

echo 使用端口: %COM_PORT%
echo 波特率: %BAUD_RATE%
echo.

cd /d "%~dp0..\"
pio run --target upload && pio device monitor --port %COM_PORT% --baud %BAUD_RATE%