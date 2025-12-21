@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

REM 接收平台参数（例如 pico32 或 esp32-s3-devkitc-1 或 esp32-s3-debug）
set "TARGET_ENV=%~1"
if "%TARGET_ENV%"=="" set "TARGET_ENV=pico32"

set "INI_FILE=%~dp0..\platformio.ini"

if not exist "%INI_FILE%" (
    echo platformio.ini not found
    exit /b 1
)

REM esp32-s3-debug 和 esp32-s3-devkitc-1 都继承自 esp32s3_common，共用端口配置
set "BASE_SECTION="
if "%TARGET_ENV%"=="esp32-s3-debug" set "BASE_SECTION=esp32s3_common"
if "%TARGET_ENV%"=="esp32-s3-devkitc-1" set "BASE_SECTION=esp32s3_common"

REM 先读取基础配置（如果有继承）
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

REM 读取目标环境的配置（可覆盖基础配置）
set "in_target=0"
for /f "usebackq tokens=1,* delims==" %%a in ("%INI_FILE%") do (
    set "line=%%a%%b"
    set "line=!line: =!"
    
    REM 检查是否进入目标环境
    if "!line!"=="[env:%TARGET_ENV%]" (
        set "in_target=1"
    ) else if "!line:~0,1!"=="[" (
        set "in_target=0"
    )
    
    REM 如果在目标环境内，读取配置
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