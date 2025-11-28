@echo off
REM SDL3设置和构建脚本
REM 使用方法：修改SDL3_ROOT为你解压SDL3的路径，然后运行此脚本

echo ====================================
echo LVGL Simulator - SDL3 Setup
echo ====================================
echo.

REM ========================================
REM 配置区：修改为你的SDL3解压路径
REM ========================================
REM 示例：set SDL3_ROOT=C:\SDL3\SDL3-3.2.26\x86_64-w64-mingw32
REM 请根据实际解压位置修改下面这行
set SDL3_ROOT=C:\SDL3\SDL3-3.2.26\x86_64-w64-mingw32

REM ========================================
REM 以下内容一般不需要修改
REM ========================================

echo Checking SDL3 installation...
echo SDL3_ROOT=%SDL3_ROOT%
echo.

REM 检查SDL3路径是否存在
if not exist "%SDL3_ROOT%" (
    echo [ERROR] SDL3 not found at: %SDL3_ROOT%
    echo.
    echo Please follow these steps:
    echo 1. Extract SDL3-devel-3.2.26-mingw.zip to C:\SDL3\
    echo 2. Update SDL3_ROOT in this script to match your extraction path
    echo 3. Run this script again
    echo.
    pause
    exit /b 1
)

REM 检查SDL3关键文件
if not exist "%SDL3_ROOT%\include\SDL3\SDL.h" (
    echo [ERROR] SDL3 headers not found!
    echo Expected: %SDL3_ROOT%\include\SDL3\SDL.h
    pause
    exit /b 1
)

if not exist "%SDL3_ROOT%\lib\libSDL3.dll.a" (
    echo [ERROR] SDL3 library not found!
    echo Expected: %SDL3_ROOT%\lib\libSDL3.dll.a
    pause
    exit /b 1
)

if not exist "%SDL3_ROOT%\bin\SDL3.dll" (
    echo [ERROR] SDL3.dll not found!
    echo Expected: %SDL3_ROOT%\bin\SDL3.dll
    pause
    exit /b 1
)

echo [OK] SDL3 installation verified
echo.

REM 设置环境变量
set SDL3_DIR=%SDL3_ROOT%
set PATH=%PATH%;%SDL3_ROOT%\bin

echo Environment variables set:
echo   SDL3_DIR=%SDL3_DIR%
echo   PATH includes SDL3 bin directory
echo.

REM 检查CMake
echo Checking build tools...
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] CMake not found!
    echo.
    echo Please install CMake first. Recommended method:
    echo.
    echo Method 1: Use MSYS2 (Recommended)
    echo   1. Install MSYS2 from https://www.msys2.org/
    echo   2. Run: pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-toolchain
    echo   3. Add C:\msys64\mingw64\bin to system PATH
    echo.
    echo Method 2: Download CMake installer
    echo   1. Download from https://cmake.org/download/
    echo   2. Install and check "Add CMake to system PATH"
    echo   3. Restart this terminal
    echo.
    echo For detailed instructions, see INSTALL_CMAKE.md
    echo.
    pause
    exit /b 1
)
echo [OK] CMake found

REM 检查MinGW
where gcc >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] MinGW GCC not found!
    echo.
    echo Please install MinGW-w64. Recommended method:
    echo.
    echo Method 1: Use MSYS2 (Recommended)
    echo   1. Install MSYS2 from https://www.msys2.org/
    echo   2. Run: pacman -S mingw-w64-x86_64-toolchain
    echo   3. Add C:\msys64\mingw64\bin to system PATH
    echo.
    echo For detailed instructions, see INSTALL_CMAKE.md
    echo.
    pause
    exit /b 1
)
echo [OK] MinGW GCC found

where mingw32-make >nul 2>nul
if %errorlevel% neq 0 (
    echo [WARNING] mingw32-make not found
    echo Will use 'cmake --build .' instead
    echo.
    set USE_CMAKE_BUILD=1
) else (
    echo [OK] mingw32-make found
    set USE_CMAKE_BUILD=0
)

echo [OK] Build tools verified
echo.

REM 创建构建目录
if not exist build mkdir build
cd build

REM 配置CMake
echo ====================================
echo Configuring CMake...
echo ====================================
cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=%SDL3_ROOT% ..
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] CMake configuration failed!
    echo Please check the error messages above.
    cd ..
    pause
    exit /b 1
)

echo.
echo [OK] CMake configuration successful
echo.

REM 编译
echo ====================================
echo Building project...
echo ====================================
cmake --build .
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Build failed!
    echo Please check the error messages above.
    cd ..
    pause
    exit /b 1
)

echo.
echo [OK] Build successful
echo.

REM 检查SDL3.dll是否已自动复制
if not exist "bin\SDL3.dll" (
    echo Copying SDL3.dll manually...
    copy "%SDL3_ROOT%\bin\SDL3.dll" "bin\" /Y
    if %errorlevel% equ 0 (
        echo [OK] SDL3.dll copied
    ) else (
        echo [WARNING] Failed to copy SDL3.dll
        echo Please copy it manually from: %SDL3_ROOT%\bin\SDL3.dll
        echo                           to: build\bin\
    )
)

echo.
echo ====================================
echo Build Complete!
echo ====================================
echo.
echo Executable location: build\bin\lvgl_simulator.exe
echo.
echo To run the simulator:
echo   cd build\bin
echo   lvgl_simulator.exe
echo.
echo Or with a custom image:
echo   lvgl_simulator.exe path\to\image.bin
echo.

cd ..
pause
