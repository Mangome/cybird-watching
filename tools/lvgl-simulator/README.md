# LVGL Windows模拟器

用于在Windows PC上预览和测试RGB565格式图片的LVGL模拟器。

## 功能特性

- ✅ 加载并显示LVGL格式的RGB565二进制图片（.bin）
- ✅ 支持240x240分辨率显示区域（可调整）
- ✅ 图片浏览控制（上一帧/下一帧）
- ✅ 实时显示图片信息（尺寸、路径、帧号）
- ✅ 鼠标交互支持
- ✅ 键盘快捷键（ESC退出）

## 系统要求

- Windows 10/11
- **CMake 3.15+**
- **MinGW-w64**
- **SDL3开发库**

### 快速安装（推荐使用MSYS2）⭐

**一站式安装所有开发工具**：

1. **下载并安装MSYS2**: https://www.msys2.org/

2. **打开MSYS2 MINGW64终端**，执行：
   ```bash
   # 更新包数据库
   pacman -Syu
   
   # 一次性安装所有工具（注意sdl3是小写）
   pacman -S mingw-w64-x86_64-toolchain \
             mingw-w64-x86_64-cmake \
             mingw-w64-x86_64-sdl3
   ```

3. **添加到系统PATH**：
   - 右键"此电脑" → "属性" → "高级系统设置" → "环境变量"
   - 在"系统变量"的"Path"中添加：`C:\msys64\mingw64\bin`

4. **验证安装**（重启终端后）：
   ```bash
   cmake --version
   gcc --version
   ```

📖 **详细说明请查看 [QUICK_START.md](QUICK_START.md)**

## 安装SDL3

### 方法1：使用MSYS2（推荐）⭐

```bash
# 在MSYS2 MINGW64终端中执行（注意sdl3是小写）
pacman -S mingw-w64-x86_64-sdl3
```

**优点**：
- ✅ 自动管理依赖和更新
- ✅ 无需手动配置路径
- ✅ 与其他工具统一管理

### 方法2：使用预编译的MinGW版本

**如果你已经下载了 `SDL3-devel-3.2.26-mingw.zip`**：

1. **解压SDL3到固定位置**：
   ```
   C:\SDL3\
   ```
   
   解压后应该有如下结构：
   ```
   C:\SDL3\SDL3-3.2.26\x86_64-w64-mingw32\
   ├── bin\SDL3.dll
   ├── include\SDL3\SDL.h
   └── lib\libSDL3.dll.a
   ```

2. **使用提供的自动化脚本**：
   
   编辑 `setup_sdl3_manual.bat`，修改SDL3路径：
   ```batch
   set SDL3_ROOT=C:\SDL3\SDL3-3.2.26\x86_64-w64-mingw32
   ```
   
   然后运行脚本即可。

3. **或者手动编译**：
   ```bash
   mkdir build
   cd build
   cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:/SDL3/SDL3-3.2.26/x86_64-w64-mingw32 ..
   cmake --build .
   copy C:\SDL3\SDL3-3.2.26\x86_64-w64-mingw32\bin\SDL3.dll bin\
   ```

📖 **详细说明请查看 [INSTALL_SDL3.md](INSTALL_SDL3.md)**

## 编译步骤

### 方法1：使用MSYS2的SDL3（推荐）⭐

如果使用MSYS2安装了所有工具，编译非常简单：

```bash
cd tools/lvgl-simulator
mkdir build
cd build

# 直接编译，不需要指定SDL3路径
cmake -G "MinGW Makefiles" ..
cmake --build .

# 运行
cd bin
lvgl_simulator.exe
```

### 方法2：使用手动下载的SDL3

如果你已下载 `SDL3-devel-3.2.26-mingw.zip`：

1. 解压到 `C:\SDL3\`
2. 编辑 `setup_sdl3_manual.bat`，设置SDL3路径
3. 运行脚本：
   ```bash
   setup_sdl3_manual.bat
   ```

### 方法3：手动编译

```bash
cd tools/lvgl-simulator
mkdir build
cd build

# 配置（如果SDL2在自定义路径，需要设置SDL2_DIR）
cmake -G "MinGW Makefiles" ..

# 或指定SDL2路径
cmake -G "MinGW Makefiles" -DSDL2_DIR=C:/path/to/SDL2 ..

# 编译
cmake --build .

# 运行
./bin/lvgl_simulator.exe
```

### 使用CMake + Visual Studio

```bash
cd tools/lvgl-simulator
mkdir build
cd build

# 配置
cmake -G "Visual Studio 16 2019" ..

# 编译
cmake --build . --config Release

# 运行
./bin/Release/lvgl_simulator.exe
```

## 使用说明

### 基本用法

```bash
# 加载默认图片（./assets/test.bin）
./lvgl_simulator.exe

# 加载指定图片
./lvgl_simulator.exe path/to/image.bin
```

### 准备测试图片

1. 使用Python转换脚本转换图片：

```bash
cd ../../scripts/converter
python -m src.converter.rgb565_cli input.png -o output.bin --max-size 240 240
```

2. 将生成的.bin文件复制到 `tools/lvgl-simulator/assets/` 目录

3. 运行模拟器查看效果

### 界面操作

- **Load按钮**: 重新加载当前图片
- **< Prev按钮**: 加载上一帧（frame{N-1}.bin）
- **Next >按钮**: 加载下一帧（frame{N+1}.bin）
- **ESC键**: 退出程序
- **鼠标**: 可以点击按钮进行交互

## 目录结构

```
lvgl-simulator/
├── CMakeLists.txt          # CMake配置文件
├── lv_conf.h              # LVGL配置文件
├── README.md              # 本文档
├── src/
│   ├── main.c            # 主程序入口
│   └── hal/              # 硬件抽象层
│       ├── sdl_display.c # SDL显示驱动
│       ├── sdl_display.h
│       ├── sdl_mouse.c   # SDL鼠标驱动
│       ├── sdl_mouse.h
│       ├── file_loader.c # 图片文件加载器
│       └── file_loader.h
├── assets/               # 资源文件目录
│   └── test.bin         # 测试图片（需手动添加）
└── build/               # 构建目录（生成）
```

## 图片格式规范

模拟器支持LVGL 7.9.1标准的RGB565二进制格式：

```
[12字节头部]
  - 4字节: cf (颜色格式=4) + always_zero (=0)
  - 4字节: width + height
  - 4字节: data_size
[N字节像素数据]
  - RGB565格式，每像素2字节
```

## 常见问题

### Q: 提示 'cmake' is not recognized

**A**: CMake未安装或未添加到PATH。

**推荐方案**：使用MSYS2一次性安装所有工具
```bash
# 在MSYS2 MINGW64终端中
pacman -S mingw-w64-x86_64-toolchain \
          mingw-w64-x86_64-cmake \
          mingw-w64-x86_64-sdl3

# 添加到系统PATH: C:\msys64\mingw64\bin
# 重启终端
```

详见 [QUICK_START.md](QUICK_START.md)

### Q: 提示找不到SDL3

**A**: 根据你的安装方式：

**如果使用MSYS2**：
```bash
# 确认已安装（注意小写sdl3）
pacman -S mingw-w64-x86_64-sdl3
```

**如果手动下载**：
```bash
# CMake指定路径
cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:/SDL3/SDL3-3.2.26/x86_64-w64-mingw32 ..
```

### Q: 图片无法加载

**A**: 检查：
1. 图片格式是否正确（使用转换脚本生成）
2. 文件路径是否正确
3. 查看控制台错误信息

### Q: 显示效果与硬件不一致

**A**: 可能原因：
1. 颜色字节序不同（检查LV_COLOR_16_SWAP配置）
2. 分辨率设置不一致
3. 显示驱动差异

## 开发调试

### 启用详细日志

修改 `lv_conf.h`:

```c
#define LV_USE_LOG      1
#define LV_LOG_LEVEL    LV_LOG_LEVEL_TRACE
```

### 修改窗口尺寸

修改 `src/main.c`:

```c
#define WINDOW_WIDTH  800  // 窗口宽度
#define WINDOW_HEIGHT 600  // 窗口高度
#define DISPLAY_HOR_RES 240 // 显示区域宽度
#define DISPLAY_VER_RES 240 // 显示区域高度
```

## 许可证

本项目基于现有项目的许可证。LVGL库采用MIT许可证。

## 相关链接

- [LVGL官网](https://lvgl.io/)
- [LVGL文档](https://docs.lvgl.io/)
- [SDL2官网](https://www.libsdl.org/)
