@echo off
chcp 65001 >nul
title CybirdWatching CLI

echo ======================================
echo   CybirdWatching CLI 快速启动器
echo ======================================
echo.

REM 选择平台
echo 请选择目标平台：
echo 1. pico32 (ESP32)
echo 2. esp32-s3-devkitc-1 (ESP32-S3)
echo.
set /p choice=请输入选项 (1-2): 

if "%choice%"=="1" (
    set PLATFORM=pico32
) else if "%choice%"=="2" (
    set PLATFORM=esp32-s3-devkitc-1
) else (
    echo 无效选择，使用默认平台 pico32
    set PLATFORM=pico32
)

echo.
echo 目标平台: %PLATFORM%

REM 读取platformio.ini配置（传递平台参数）
call "%~dp0read_platformio.bat" %PLATFORM%
if "%COM_PORT%"=="" (
    echo 警告: 无法读取配置，使用默认端口 COM3
    set "COM_PORT=COM3"
)

echo 使用端口: %COM_PORT%
echo.

REM 检查是否在正确目录
if not exist "cybird_watching_cli\src\cybird_watching_cli\main.py" (
    echo 错误: 找不到cybird_watching_cli目录
    echo 请确保此bat文件位于scripts目录中
    pause
    exit /b 1
)

echo 正在启动CybirdWatching CLI交互模式...
echo.

REM 进入CLI目录并启动
cd /d cybird_watching_cli

REM 检查uv是否安装
where uv >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo 错误: 未找到uv包管理器
    echo 请先安装uv: https://docs.astral.sh/uv/
    pause
    exit /b 1
)

REM 启动CLI (传递平台信息)
echo.
echo 启动参数: 
echo   - 串口: %COM_PORT%
echo   - 平台: %PLATFORM%
echo.
uv run python -m cybird_watching_cli.main -p %COM_PORT% --platform %PLATFORM%

echo.
echo 感谢使用CybirdWatching CLI！
pause