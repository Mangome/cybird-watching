@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

set "INI_FILE=%~dp0..\platformio.ini"

if not exist "%INI_FILE%" (
    echo platformio.ini not found
    exit /b 1
)

REM 第一遍：读取 [common] 部分的值
set "in_common=0"
for /f "usebackq tokens=1,* delims==" %%a in ("%INI_FILE%") do (
    set "line=%%a%%b"
    set "line=!line: =!"
    
    if "!line!"=="[common]" (
        set "in_common=1"
    ) else if "!line:~0,1!"=="[" (
        set "in_common=0"
    )
    
    if "!in_common!"=="1" (
        set "key=%%a"
        set "value=%%b"
        set "key=!key: =!"
        set "value=!value: =!"
        
        if "!key!"=="monitor_speed" set "common_monitor_speed=!value!"
        if "!key!"=="upload_port" set "common_upload_port=!value!"
    )
)

REM 第二遍：读取所有配置，并解析变量引用
for /f "usebackq tokens=1,* delims==" %%a in ("%INI_FILE%") do (
    set "key=%%a"
    set "value=%%b"
    set "key=!key: =!"
    set "value=!value: =!"
    
    REM 解析 ${common.monitor_speed}
    if "!value!"=="${common.monitor_speed}" set "value=!common_monitor_speed!"
    if "!value!"=="${common.upload_port}" set "value=!common_upload_port!"
    
    if "!key!"=="upload_port" set "COM_PORT=!value!"
    if "!key!"=="monitor_speed" set "BAUD_RATE=!value!"
)

endlocal & set "COM_PORT=%COM_PORT%" & set "BAUD_RATE=%BAUD_RATE%"