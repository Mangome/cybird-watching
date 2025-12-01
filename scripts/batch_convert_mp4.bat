@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Mp4 Converter Tool
echo ========================================
echo.

:: Get source directory
set "SOURCE_DIR="
set /p "SOURCE_DIR=Enter source directory path: "
if not exist "!SOURCE_DIR!" (
    echo Error: Source directory does not exist!
    pause
    exit /b 1
)

:: Get target directory
set "TARGET_DIR="
set /p "TARGET_DIR=Enter target directory path: " 

echo.
echo Source directory: !SOURCE_DIR!
echo Target directory: !TARGET_DIR!
echo.

:: Confirm execution
set "CONFIRM="
set /p "CONFIRM=Confirm conversion? (Y/N): "
if /i not "!CONFIRM!"=="Y" (
    echo Operation cancelled
    pause
    exit /b 0
)

echo.
echo Starting conversion...
echo ========================================

:: Switch to converter directory and execute commands
cd /d "%~dp0mp4converter"
if errorlevel 1 (
    echo Error: Cannot switch to converter directory!
    pause
    exit /b 1
)

echo Converting images...
uv run mp4-converter batch "!SOURCE_DIR!" "!TARGET_DIR!" --output-format rgb565 --frame-rate 8 --resize 120x120 --workers 8

if errorlevel 1 (
    echo.
    echo Conversion failed!
    pause
    exit /b 1
) else (
    echo.
    echo Conversion completed!
)

pause
