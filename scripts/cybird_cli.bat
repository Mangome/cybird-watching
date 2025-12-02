@echo off
chcp 65001 >nul
title CybirdWatching CLI

echo ======================================
echo   CybirdWatching CLI 快速启动器
echo ======================================
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

REM 启动CLI
uv run python -m cybird_watching_cli.main -p COM5

echo.
echo 感谢使用CybirdWatching CLI！
pause