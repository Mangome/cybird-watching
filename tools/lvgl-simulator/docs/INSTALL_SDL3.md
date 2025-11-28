# SDL3手动安装指南

## 使用下载的SDL3-devel-3.2.26-mingw.zip

### 步骤1：解压SDL3

将下载的 `SDL3-devel-3.2.26-mingw.zip` 解压到一个固定位置，例如：

```
C:\SDL3\
```

解压后的目录结构应该类似：
```
C:\SDL3\
├── SDL3-3.2.26\
│   ├── x86_64-w64-mingw32\     # 64位版本
│   │   ├── bin\
│   │   │   └── SDL3.dll
│   │   ├── include\
│   │   │   └── SDL3\
│   │   │       ├── SDL.h
│   │   │       └── ...
│   │   ├── lib\
│   │   │   ├── libSDL3.a
│   │   │   ├── libSDL3.dll.a
│   │   │   └── cmake\
│   │   │       └── SDL3\
│   │   └── share\
│   └── i686-w64-mingw32\       # 32位版本（如果需要）
```

### 步骤2：设置环境变量（可选但推荐）

**方法A：系统环境变量（永久）**

1. 右键"此电脑" → "属性" → "高级系统设置" → "环境变量"
2. 在"系统变量"中新建：
   - 变量名：`SDL3_DIR`
   - 变量值：`C:\SDL3\SDL3-3.2.26\x86_64-w64-mingw32`
3. 编辑"Path"变量，添加：
   - `C:\SDL3\SDL3-3.2.26\x86_64-w64-mingw32\bin`

**方法B：临时设置（当前终端会话）**

在PowerShell中：
```powershell
$env:SDL3_DIR = "C:\SDL3\SDL3-3.2.26\x86_64-w64-mingw32"
$env:PATH += ";C:\SDL3\SDL3-3.2.26\x86_64-w64-mingw32\bin"
```

在CMD中：
```cmd
set SDL3_DIR=C:\SDL3\SDL3-3.2.26\x86_64-w64-mingw32
set PATH=%PATH%;C:\SDL3\SDL3-3.2.26\x86_64-w64-mingw32\bin
```

### 步骤3：编译项目

#### 方法1：使用环境变量（推荐）

如果已设置 `SDL3_DIR` 环境变量：

```bash
cd tools/lvgl-simulator
mkdir build
cd build

# CMake会自动找到SDL3
cmake -G "MinGW Makefiles" ..
cmake --build .
```

#### 方法2：手动指定SDL3路径

```bash
cd tools/lvgl-simulator
mkdir build
cd build

# 指定SDL3的cmake配置路径
cmake -G "MinGW Makefiles" \
  -DCMAKE_PREFIX_PATH="C:/SDL3/SDL3-3.2.26/x86_64-w64-mingw32" \
  ..

cmake --build .
```

#### 方法3：使用提供的批处理脚本

修改 `build_windows.bat`，在cmake命令前添加SDL3路径：

```batch
cmake -G "MinGW Makefiles" ^
  -DCMAKE_PREFIX_PATH=C:/SDL3/SDL3-3.2.26/x86_64-w64-mingw32 ^
  ..
```

然后运行：
```bash
build_windows.bat
```

### 步骤4：复制SDL3.dll（重要！）

编译成功后，需要将SDL3.dll复制到可执行文件目录：

```bash
# 方法1：手动复制
copy C:\SDL3\SDL3-3.2.26\x86_64-w64-mingw32\bin\SDL3.dll build\bin\

# 方法2：在CMake中自动复制（推荐，见下方）
```

### 步骤5：运行程序

```bash
cd build\bin
lvgl_simulator.exe
```

## 自动化脚本

为了简化流程，可以创建一个配置脚本。

### setup_sdl3.bat

在 `tools/lvgl-simulator/` 创建此文件：

```batch
@echo off
echo Setting up SDL3 for LVGL Simulator
echo ====================================

REM 设置SDL3路径（根据你的解压位置修改）
set SDL3_ROOT=C:\SDL3\SDL3-3.2.26\x86_64-w64-mingw32

REM 检查路径是否存在
if not exist "%SDL3_ROOT%" (
    echo Error: SDL3 not found at %SDL3_ROOT%
    echo Please extract SDL3-devel-3.2.26-mingw.zip and update SDL3_ROOT
    pause
    exit /b 1
)

REM 设置环境变量
set SDL3_DIR=%SDL3_ROOT%
set PATH=%PATH%;%SDL3_ROOT%\bin

echo SDL3_DIR=%SDL3_DIR%
echo PATH updated

REM 创建构建目录
if not exist build mkdir build
cd build

REM 配置CMake
echo.
echo Configuring CMake...
cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=%SDL3_ROOT% ..
if %errorlevel% neq 0 (
    echo Error: CMake configuration failed!
    pause
    exit /b 1
)

REM 编译
echo.
echo Building...
cmake --build .
if %errorlevel% neq 0 (
    echo Error: Build failed!
    pause
    exit /b 1
)

REM 复制SDL3.dll
echo.
echo Copying SDL3.dll...
copy "%SDL3_ROOT%\bin\SDL3.dll" "bin\" /Y

echo.
echo ====================================
echo Build successful!
echo Executable: build\bin\lvgl_simulator.exe
echo ====================================

cd ..
pause
```

## CMakeLists.txt优化（自动复制DLL）

可以在CMakeLists.txt中添加自动复制SDL3.dll的逻辑：

```cmake
# 在文件末尾添加
if(WIN32)
    # 查找SDL3.dll
    find_file(SDL3_DLL
        NAMES SDL3.dll
        PATHS
            ${CMAKE_PREFIX_PATH}/bin
            $ENV{SDL3_DIR}/bin
        NO_DEFAULT_PATH
    )
    
    if(SDL3_DLL)
        message(STATUS "Found SDL3.dll: ${SDL3_DLL}")
        # 复制到构建目录
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SDL3_DLL}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMENT "Copying SDL3.dll to output directory"
        )
    else()
        message(WARNING "SDL3.dll not found, you may need to copy it manually")
    endif()
endif()
```

## 验证安装

运行以下命令检查SDL3是否正确配置：

```bash
# 检查SDL3头文件
dir C:\SDL3\SDL3-3.2.26\x86_64-w64-mingw32\include\SDL3\SDL.h

# 检查SDL3库文件
dir C:\SDL3\SDL3-3.2.26\x86_64-w64-mingw32\lib\libSDL3.dll.a

# 检查SDL3 DLL
dir C:\SDL3\SDL3-3.2.26\x86_64-w64-mingw32\bin\SDL3.dll
```

## 常见问题

### Q: 编译时提示找不到SDL3

**A**: 检查CMAKE_PREFIX_PATH是否正确指向SDL3的根目录（包含lib/cmake/SDL3的目录）

### Q: 运行时提示缺少SDL3.dll

**A**: 将 `SDL3.dll` 复制到可执行文件同目录，或将SDL3的bin目录添加到PATH

### Q: 链接错误 undefined reference to SDL_xxx

**A**: 确保链接了正确的SDL3库，检查CMakeLists.txt中的 `target_link_libraries`

## 目录结构参考

正确解压后的SDL3目录应该是：

```
C:\SDL3\SDL3-3.2.26\x86_64-w64-mingw32\
├── bin\
│   └── SDL3.dll              ← 运行时需要
├── include\
│   └── SDL3\
│       ├── SDL.h             ← 主头文件
│       ├── SDL_events.h
│       └── ...
├── lib\
│   ├── libSDL3.dll.a         ← 动态链接库
│   ├── libSDL3.a             ← 静态库
│   └── cmake\
│       └── SDL3\
│           └── SDL3Config.cmake  ← CMake配置文件
└── share\
```

如果你的解压路径不同，请相应调整所有命令中的路径。
