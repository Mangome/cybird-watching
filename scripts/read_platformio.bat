@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

REM Receive platform parameter (e.g. pico32 or esp32-s3-devkitc-1 or esp32-s3-debug)
set "TARGET_ENV=%~1"
if "%TARGET_ENV%"=="" set "TARGET_ENV=pico32"

set "INI_FILE=%~dp0..\platformio.ini"

if not exist "%INI_FILE%" (
    echo platformio.ini not found
    exit /b 1
)

REM esp32-s3-debug and esp32-s3-devkitc-1 both inherit from esp32s3_common, share port configuration
set "BASE_SECTION="
if "%TARGET_ENV%"=="esp32-s3-debug" set "BASE_SECTION=esp32s3_common"
if "%TARGET_ENV%"=="esp32-s3-devkitc-1" set "BASE_SECTION=esp32s3_common"

REM Read base configuration first (if inherited)
if not "%BASE_SECTION%"=="" (
    set "in_target=0"
    for /f "usebackq tokens=1,* delims==" %%a in ("%INI_FILE%") do (
        set "line=%%a%%b"
        set "line=!line: =!"
        
        if "!line!"=="[%BASE_SECTION%]" (
            set "in_target=1"
        ) else if "!line:~0,1!"=="[" (
            set "in_target=0"
        )
        
        if "!in_target!"=="1" (
            set "key=%%a"
            set "value=%%b"
            set "key=!key: =!"
            set "value=!value: =!"
            
            if "!key!"=="upload_port" set "COM_PORT=!value!"
            if "!key!"=="monitor_port" set "COM_PORT=!value!"
            if "!key!"=="monitor_speed" set "BAUD_RATE=!value!"
        )
    )
)

REM Read target environment configuration (can override base configuration)
set "in_target=0"
for /f "usebackq tokens=1,* delims==" %%a in ("%INI_FILE%") do (
    set "line=%%a%%b"
    set "line=!line: =!"
    
    REM Check if entering target environment
    if "!line!"=="[env:%TARGET_ENV%]" (
        set "in_target=1"
    ) else if "!line:~0,1!"=="[" (
        set "in_target=0"
    )
    
    REM If within target environment, read configuration
    if "!in_target!"=="1" (
        set "key=%%a"
        set "value=%%b"
        set "key=!key: =!"
        set "value=!value: =!"
        
        if "!key!"=="upload_port" set "COM_PORT=!value!"
        if "!key!"=="monitor_port" set "COM_PORT=!value!"
        if "!key!"=="monitor_speed" set "BAUD_RATE=!value!"
    )
)

endlocal & set "COM_PORT=%COM_PORT%" & set "BAUD_RATE=%BAUD_RATE%"