# CMake和MinGW安装指南

## 问题：cmake命令未找到

如果遇到 `cmake: The term 'cmake' is not recognized...` 错误，说明CMake未安装或未添加到系统PATH。

## 解决方案

### 方法1：使用MSYS2安装（强烈推荐）⭐

MSYS2提供完整的开发环境，包括CMake、MinGW、SDL3等所有需要的工具。

#### 步骤1：安装MSYS2

1. 访问 https://www.msys2.org/
2. 下载安装器：`msys2-x86_64-*.exe`
3. 运行安装器，安装到默认位置：`C:\msys64`

#### 步骤2：更新MSYS2

安装完成后，MSYS2会自动打开终端窗口，执行：

```bash
pacman -Syu
```

如果提示关闭窗口，关闭后重新打开"MSYS2 MINGW64"（从开始菜单），再次运行：

```bash
pacman -Su
```

#### 步骤3：安装开发工具

在MSYS2 MINGW64终端中执行：

```bash
# 安装基础开发工具
pacman -S --needed base-devel mingw-w64-x86_64-toolchain

# 安装CMake
pacman -S mingw-w64-x86_64-cmake

# 安装SDL3（注意：包名是小写sdl3）
pacman -S mingw-w64-x86_64-sdl3

# 安装Make
pacman -S mingw-w64-x86_64-make

# 安装Git（可选）
pacman -S git
```

#### 步骤4：添加到系统PATH

将MSYS2的MinGW64 bin目录添加到系统PATH：

1. 右键"此电脑" → "属性" → "高级系统设置" → "环境变量"
2. 在"系统变量"中找到"Path"，点击"编辑"
3. 点击"新建"，添加：
   ```
   C:\msys64\mingw64\bin
   ```
4. 确定保存

#### 步骤5：验证安装

**重新打开PowerShell或CMD**（必须重启终端），然后测试：

```powershell
# 测试CMake
cmake --version
# 应该显示：cmake version 3.x.x

# 测试MinGW
gcc --version
# 应该显示：gcc (Rev...) x.x.x

# 测试Make
mingw32-make --version
# 应该显示：GNU Make x.x
```

#### 步骤6：编译项目

```bash
cd tools/lvgl-simulator
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
cmake --build .
```

---

### 方法2：使用Chocolatey（Windows包管理器）

如果你熟悉命令行包管理器：

#### 步骤1：安装Chocolatey

以管理员权限打开PowerShell，执行：

```powershell
Set-ExecutionPolicy Bypass -Scope Process -Force
[System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
```

#### 步骤2：安装CMake和MinGW

```powershell
choco install cmake -y
choco install mingw -y
```

#### 步骤3：刷新环境变量

```powershell
refreshenv
```

或重新打开PowerShell。

---

### 方法3：手动下载安装

#### 安装CMake

1. 访问 https://cmake.org/download/
2. 下载 `cmake-3.x.x-windows-x86_64.msi`
3. 安装时**勾选"Add CMake to the system PATH"**
4. 完成安装后重启终端

#### 安装MinGW-w64

1. 访问 https://github.com/niXman/mingw-builds-binaries/releases
2. 下载 `x86_64-*-release-posix-seh-*.7z`
3. 解压到 `C:\mingw64`
4. 手动添加到PATH：`C:\mingw64\bin`

---

## 快速检查清单

### 必需工具

- [ ] **CMake** (3.15+)
  ```bash
  cmake --version
  ```

- [ ] **MinGW GCC**
  ```bash
  gcc --version
  ```

- [ ] **Make**
  ```bash
  mingw32-make --version
  ```

- [ ] **SDL3**（库文件）
  - 通过MSYS2安装：`pacman -S mingw-w64-x86_64-SDL3`
  - 或手动解压：`SDL3-devel-3.2.26-mingw.zip`

### 验证PATH

```powershell
# 检查PATH中是否包含必要目录
$env:PATH -split ';' | Select-String -Pattern 'mingw|cmake'
```

应该包含类似：
```
C:\msys64\mingw64\bin
C:\Program Files\CMake\bin
```

---

## 使用MSYS2的完整流程（推荐）

### 一次性安装所有工具

```bash
# 在MSYS2 MINGW64终端中执行（注意sdl3是小写）
pacman -S --needed \
  mingw-w64-x86_64-toolchain \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-sdl3
```

### 编译LVGL模拟器

**在MSYS2 MINGW64终端中**（不是Windows PowerShell）：

```bash
cd /e/Projects/cybird-watching/cybird-watching/tools/lvgl-simulator
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
cmake --build .
```

### 或在Windows PowerShell/CMD中

确保PATH包含 `C:\msys64\mingw64\bin`：

```powershell
cd E:\Projects\cybird-watching\cybird-watching\tools\lvgl-simulator
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
cmake --build .
```

---

## 常见问题

### Q: 为什么推荐MSYS2？

**A**: 
- ✅ 一站式解决方案（CMake + MinGW + SDL3）
- ✅ 包管理器方便更新和管理
- ✅ 与Linux开发体验一致
- ✅ 避免DLL地狱和版本冲突

### Q: cmake命令仍然不识别

**A**: 确保：
1. 已安装CMake
2. 已添加到系统PATH
3. **已重启终端**（非常重要！）
4. 使用正确的终端（MSYS2 MINGW64或新开的PowerShell）

### Q: mingw32-make找不到

**A**: 
- MSYS2用户：`pacman -S mingw-w64-x86_64-make`
- 或使用cmake构建：`cmake --build .`

### Q: 手动安装的SDL3如何与MSYS2共存？

**A**: 
- 推荐统一使用MSYS2安装：`pacman -S mingw-w64-x86_64-SDL3`
- 或在CMake中指定路径：`-DCMAKE_PREFIX_PATH=C:/SDL3/...`

---

## 推荐的完整开发环境配置

### 选项A：纯MSYS2（推荐新手）

```bash
# 1. 安装MSYS2
# 2. 在MSYS2 MINGW64中：
pacman -S mingw-w64-x86_64-toolchain \
          mingw-w64-x86_64-cmake \
          mingw-w64-x86_64-SDL3

# 3. 添加 C:\msys64\mingw64\bin 到系统PATH
# 4. 在任意终端编译项目
```

### 选项B：手动安装（高级用户）

```
1. CMake (msi安装，勾选PATH)
2. MinGW-w64 (解压，手动添加PATH)
3. SDL3 (解压，使用CMAKE_PREFIX_PATH)
```

---

## 下一步

安装完成后，运行：

```bash
cd tools/lvgl-simulator
setup_sdl3.bat
```

或查看 [INSTALL_SDL3.md](INSTALL_SDL3.md) 继续SDL3配置。
