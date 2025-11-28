# 快速开始指南

## LVGL Windows模拟器 - 完整安装指南

本指南提供两种安装方案，推荐使用方案A（MSYS2一站式安装）。

---

## 方案A：使用MSYS2一站式安装（推荐）⭐

使用MSYS2统一管理所有开发工具（CMake、MinGW、SDL3）。

**优点**：
- ✅ 一次安装所有工具
- ✅ 自动管理依赖和更新
- ✅ 无需手动配置路径
- ✅ 最简单快捷

**缺点**：
- SDL3版本可能比手动下载的稍旧（3.2.20 vs 3.2.26）

---

## 方案B：MSYS2 + 手动SDL3

使用MSYS2安装CMake和MinGW，使用手动下载的SDL3-3.2.26。

**优点**：
- ✅ 使用最新版SDL3（3.2.26）
- ✅ 灵活控制SDL3版本

**缺点**：
- 需要手动配置SDL3路径
- 更新SDL3需要手动下载

---

---

## 🚀 方案A：MSYS2一站式安装（推荐）

### 步骤1：安装MSYS2

1. 访问 https://www.msys2.org/
2. 下载安装器：`msys2-x86_64-*.exe`
3. 运行安装，使用默认路径：`C:\msys64`

### 步骤2：更新MSYS2

安装完成后会自动打开终端，执行：

```bash
pacman -Syu
```

如果提示关闭窗口，关闭后从开始菜单打开 **"MSYS2 MINGW64"**，再次运行：

```bash
pacman -Su
```

### 步骤3：安装所有开发工具

在MSYS2 MINGW64终端中执行：

```bash
# 一次性安装所有工具（注意sdl3是小写）
pacman -S mingw-w64-x86_64-toolchain \
          mingw-w64-x86_64-cmake \
          mingw-w64-x86_64-sdl3
```

**说明**：
- 当询问 `Enter a selection (default=all):` 时，直接按**回车**安装全部
- `toolchain` 包含 gcc、make等编译工具
- `cmake` 是构建系统
- `sdl3` 是SDL3库（注意是小写）

### 步骤4：添加MSYS2到系统PATH

1. 右键"此电脑" → "属性" → "高级系统设置" → "环境变量"
2. 在"系统变量"的"Path"中点击"编辑"
3. 点击"新建"，添加：
   ```
   C:\msys64\mingw64\bin
   ```
4. 点击"确定"保存所有对话框
5. **关闭并重新打开PowerShell或CMD**

### 步骤5：验证安装

**重新打开PowerShell**，测试：

```powershell
# 测试CMake
cmake --version
# 应输出: cmake version 3.x.x

# 测试GCC
gcc --version
# 应输出: gcc (Rev...) x.x.x

# 测试Make
mingw32-make --version
# 应输出: GNU Make x.x
```

### 步骤6：编译项目

```powershell
cd E:\Projects\cybird-watching\cybird-watching\tools\lvgl-simulator
mkdir build
cd build

# 配置（SDL3会自动找到，无需指定路径）
cmake -G "MinGW Makefiles" ..

# 编译
cmake --build .
```

### 步骤7：运行模拟器

```powershell
cd bin
lvgl_simulator.exe
```

**方案A到此结束！** ✅

---

## 🔧 方案B：MSYS2 + 手动SDL3

如果你想使用手动下载的SDL3-3.2.26（而不是MSYS2的3.2.20）。

### 步骤1-4：与方案A相同

按照方案A的步骤1-4安装MSYS2和配置PATH，但**跳过SDL3的安装**：

```bash
# 只安装这些，不安装sdl3
pacman -S mingw-w64-x86_64-toolchain \
          mingw-w64-x86_64-cmake
```

### 步骤5：解压手动下载的SDL3

将 `SDL3-devel-3.2.26-mingw.zip` 解压到：
```
C:\SDL3\
```

解压后应该有：
```
C:\SDL3\SDL3-3.2.26\x86_64-w64-mingw32\
├── bin\SDL3.dll
├── include\SDL3\SDL.h
└── lib\libSDL3.dll.a
```

### 步骤6：使用脚本编译

```powershell
cd E:\Projects\cybird-watching\cybird-watching\tools\lvgl-simulator

# 编辑 setup_sdl3_manual.bat，设置SDL3路径
# 然后运行
setup_sdl3_manual.bat
```

或手动编译：

```powershell
mkdir build && cd build
cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:/SDL3/SDL3-3.2.26/x86_64-w64-mingw32 ..
cmake --build .
copy C:\SDL3\SDL3-3.2.26\x86_64-w64-mingw32\bin\SDL3.dll bin\
```

---

## 📋 安装清单

### ✅ 方案A检查清单

- [ ] MSYS2已安装
- [ ] toolchain已安装：`gcc --version`
- [ ] CMake已安装：`cmake --version`
- [ ] SDL3已安装：`pacman -Q mingw-w64-x86_64-sdl3`
- [ ] PATH包含：`C:\msys64\mingw64\bin`
- [ ] 已重启终端

### ✅ 方案B检查清单

- [ ] MSYS2已安装
- [ ] toolchain已安装：`gcc --version`
- [ ] CMake已安装：`cmake --version`
- [ ] SDL3已解压到：`C:\SDL3\SDL3-3.2.26\x86_64-w64-mingw32\`
- [ ] PATH包含：`C:\msys64\mingw64\bin`
- [ ] 已重启终端

---

## 🔍 故障排查

### 问题1：cmake命令未找到

**症状**：`cmake: The term 'cmake' is not recognized`

**解决**：
1. 确认已安装：`pacman -S mingw-w64-x86_64-cmake`
2. 确认PATH包含：`C:\msys64\mingw64\bin`
3. **重启终端**（非常重要！）

### 问题2：gcc命令未找到

**解决**：
```bash
pacman -S mingw-w64-x86_64-toolchain
```

### 问题3：SDL3.dll缺失（方案B）

**症状**：运行时提示找不到SDL3.dll

**解决**：
```bash
# 方案A：SDL3.dll已在PATH中，应该不会出现此问题

# 方案B：手动复制
copy C:\SDL3\SDL3-3.2.26\x86_64-w64-mingw32\bin\SDL3.dll build\bin\
```

### 问题4：CMake找不到SDL3

**症状**：`SDL3 not found`

**方案A**：
```bash
# 确认SDL3已安装
pacman -Q mingw-w64-x86_64-sdl3

# 如果未安装
pacman -S mingw-w64-x86_64-sdl3
```

**方案B**：
```bash
# 确保使用 -DCMAKE_PREFIX_PATH 参数
cmake -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:/SDL3/SDL3-3.2.26/x86_64-w64-mingw32 ..
```

---

## 💡 两种方案对比

### 方案A：MSYS2一站式

| 项目 | 说明 |
|-----|-----|
| SDL3版本 | 3.2.20 |
| 安装难度 | ⭐ 最简单 |
| 管理更新 | `pacman -Syu` 一键更新 |
| 路径配置 | 自动，无需手动配置 |
| 推荐程度 | ⭐⭐⭐⭐⭐ 强烈推荐 |

### 方案B：手动SDL3

| 项目 | 说明 |
|-----|-----|
| SDL3版本 | 3.2.26（更新） |
| 安装难度 | ⭐⭐ 需要手动配置 |
| 管理更新 | 手动下载新版本 |
| 路径配置 | 需要指定CMAKE_PREFIX_PATH |
| 推荐程度 | ⭐⭐⭐ 如需特定版本 |

**建议**：除非你需要SDL3 3.2.26的特定功能，否则使用方案A即可。

---

## 📦 目录结构参考

正确安装后的结构：

```
C:\msys64\mingw64\bin\
├── cmake.exe          ← CMake
├── gcc.exe            ← 编译器
├── mingw32-make.exe   ← Make工具
└── ...

C:\SDL3\SDL3-3.2.26\x86_64-w64-mingw32\
├── bin\SDL3.dll       ← 运行时库
├── include\SDL3\      ← 头文件
└── lib\               ← 链接库

E:\Projects\cybird-watching\cybird-watching\tools\lvgl-simulator\
├── build\             ← 构建目录
│   └── bin\
│       ├── lvgl_simulator.exe
│       └── SDL3.dll   ← 自动复制
├── src\
├── CMakeLists.txt
└── setup_sdl3.bat
```

---

## 🎯 后续使用

### 方案A：直接编译

```powershell
cd E:\Projects\cybird-watching\cybird-watching\tools\lvgl-simulator
cd build
cmake --build .
cd bin
lvgl_simulator.exe
```

### 方案B：使用脚本

```powershell
cd E:\Projects\cybird-watching\cybird-watching\tools\lvgl-simulator
.\setup_sdl3_manual.bat
```

---

## 相关文档

- [INSTALL_CMAKE.md](INSTALL_CMAKE.md) - CMake和MinGW详细安装指南
- [INSTALL_SDL3.md](INSTALL_SDL3.md) - SDL3手动安装详细说明
- [README.md](README.md) - 项目完整文档
