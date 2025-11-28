@echo off
REM Windows构建脚本 - 使用MinGW

echo ====================================
echo LVGL Simulator - Windows Build
echo ====================================
echo.

REM 检查CMake是否安装
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: CMake not found! Please install CMake first.
    pause
    exit /b 1
)

REM 检查MinGW是否安装
where mingw32-make >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: MinGW not found! Please install MinGW-w64 first.
    pause
    exit /b 1
)

REM 创建构建目录
if not exist build mkdir build
cd build

REM 配置项目
echo Configuring project...
cmake -G "MinGW Makefiles" ..
if %errorlevel% neq 0 (
    echo Error: CMake configuration failed!
    pause
    exit /b 1
)

REM 编译项目
echo.
echo Building project...
cmake --build .
if %errorlevel% neq 0 (
    echo Error: Build failed!
    pause
    exit /b 1
)

echo.
echo ====================================
echo Build successful!
echo Executable: build\bin\lvgl_simulator.exe
echo ====================================
echo.

REM 返回原目录
cd ..

pause
