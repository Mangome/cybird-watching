@echo off
setlocal enabledelayedexpansion

:: 读取platformio.ini中的配置
set "INI_FILE=%~dp0..\platformio.ini"

if not exist "%INI_FILE%" (
    echo 错误: 找不到 platformio.ini
    exit /b 1
)

:: 读取upload_port和monitor_speed
for /f "usebackq tokens=1,* delims==" %%a in ("%INI_FILE%") do (
    set "key=%%a"
    set "value=%%b"
    :: 去除空格
    set "key=!key: =!"
    set "value=!value: =!"
    
    if "!key!"=="upload_port" (
        set "COM_PORT=!value!"
    )
    if "!key!"=="monitor_speed" (
        set "BAUD_RATE=!value!"
    )
)

:: 导出变量供调用脚本使用
endlocal & set "COM_PORT=%COM_PORT%" & set "BAUD_RATE=%BAUD_RATE%"