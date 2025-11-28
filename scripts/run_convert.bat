@echo off
setlocal enabledelayedexpansion

echo ========================================
echo Image Converter Tool
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
cd /d "%~dp0converter"
if errorlevel 1 (
    echo Error: Cannot switch to converter directory!
    pause
    exit /b 1
)

:: Reinstall converter package
echo Syncing dependencies...
uv sync --reinstall-package converter
if errorlevel 1 (
    echo Error: Dependency sync failed!
    pause
    exit /b 1
)

:: Run conversion
echo.
echo Converting images...
uv run converter convert "!SOURCE_DIR!" "!TARGET_DIR!"

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
