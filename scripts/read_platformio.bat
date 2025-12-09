@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

set "INI_FILE=%~dp0..\platformio.ini"

if not exist "%INI_FILE%" (
    echo platformio.ini not found
    exit /b 1
)

for /f "usebackq tokens=1,* delims==" %%a in ("%INI_FILE%") do (
    set "key=%%a"
    set "value=%%b"
    set "key=!key: =!"
    set "value=!value: =!"
    
    if "!key!"=="upload_port" (
        set "COM_PORT=!value!"
    )
    if "!key!"=="monitor_speed" (
        set "BAUD_RATE=!value!"
    )
)

endlocal & set "COM_PORT=%COM_PORT%" & set "BAUD_RATE=%BAUD_RATE%"