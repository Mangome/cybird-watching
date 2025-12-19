# CybirdWatching CLI 快速启动工具

本目录包含了用于快速启动CybirdWatching CLI的批处理文件，让您无需手动输入长命令。

## 📁 文件说明

### `cybird_cli.bat` - 统一CLI启动器
**支持多平台选择的一体化工具**

```bash
# 双击运行或命令行执行
cybird_cli.bat
```

**功能特性：**
- 🎯 **多平台支持**：启动时可选择目标平台
  - `pico32` (ESP32)
  - `esp32-s3-devkitc-1` (ESP32-S3 发布版)
  - `esp32-s3-debug` (ESP32-S3 调试版)
- 📡 **自动配置读取**：从 `platformio.ini` 读取对应平台的端口和波特率配置
- 💬 **交互式模式**：进入全功能命令行界面
- 📝 **命令历史**：支持上下键浏览历史命令
- 🔄 **智能重连**：支持设备断线重连

> ⚠️ **重要提示**：目前在ESP32-S3平台下，**只有调试版（esp32-s3-debug）支持串口命令功能**。发布版（esp32-s3-devkitc-1）暂不支持CLI交互。建议开发和调试时使用调试版。


## 🚀 快速开始

### 首次使用
1. 确保已安装 [uv](https://docs.astral.sh/uv/)
2. 将ESP32设备连接到电脑
3. 双击 `cybird_cli.bat` 启动
4. 根据提示选择目标平台（1-3）

### 交互模式基本操作

启动CLI后，您将进入交互式命令行界面：

```bash
======================================
  CybirdWatching CLI 快速启动器
======================================

请选择目标平台：
1. pico32 (ESP32)
2. esp32-s3-devkitc-1 (ESP32-S3 发布版)
3. esp32-s3-debug (ESP32-S3 调试版)

请输入选项 (1-3): 1

目标平台: pico32
使用端口: COM3

正在启动CybirdWatching CLI交互模式...
```

### 可用命令分类

#### 📟 本地命令（CLI工具内置）
- `help` - 显示帮助信息
- `quit` / `exit` - 退出CLI
- `cls` - 清屏
- `info` - 显示设备连接信息
- `reconnect` - 重新连接设备
- `test` - 测试连接（发送help命令）
- `reset` - 重置观鸟统计数据

#### 📁 文件传输命令
```bash
# 上传文件到设备
upload <本地路径> <远程路径>
file upload <本地路径> <远程路径>

# 从设备下载文件
download <远程路径> <本地路径>
file download <远程路径> <本地路径>

# 示例
upload ./config.json /configs/bird_config.json
download /configs/bird_config.json ./downloaded_config.json
```

#### 🤖 设备命令（发送到ESP32执行）
```bash
# 系统命令
status                    # 查看设备状态
help                      # 查看设备端命令帮助
file list <路径>          # 列出设备文件

# 日志命令
log                       # 查看最新日志
log lines <数量>          # 查看指定行数的日志
log clear                 # 清空日志

# 观鸟命令
bird trigger              # 触发随机小鸟动画
bird trigger <ID>         # 触发指定小鸟（如 bird trigger 1001）
bird stats                # 查看观鸟统计
bird list                 # 显示可用小鸟列表
bird reset                # 重置统计数据
```

### 使用Python直接调用

CLI工具也支持通过Python直接调用：

```bash
# 进入CLI目录
cd cybird_watching_cli

# 交互式模式
uv run python -m cybird_watching_cli.main -p COM3 --platform pico32

# 发送单个命令
uv run python -m cybird_watching_cli.main -p COM3 --platform pico32 send "status"
uv run python -m cybird_watching_cli.main -p COM3 send "log"
uv run python -m cybird_watching_cli.main -p COM4 send "bird trigger"

# 查看帮助
uv run python -m cybird_watching_cli.main --help
```

### 常用操作示例

```bash
# 1. 启动交互模式并选择平台
cybird_cli.bat

# 2. 在交互模式中使用命令：
[ON] CybirdWatching> status           # 查看设备状态
[ON] CybirdWatching> log              # 查看日志
[ON] CybirdWatching> bird trigger     # 触发小鸟动画
[ON] CybirdWatching> bird stats       # 查看观鸟统计
[ON] CybirdWatching> upload ./config.json /configs/bird_config.json  # 上传文件
[ON] CybirdWatching> download /configs/bird_config.json ./backup.json  # 下载文件
[ON] CybirdWatching> quit             # 退出
```


## ⚙️ 配置说明

### 平台配置

CLI工具从 `platformio.ini` 读取平台配置，包括：
- 串口端口 (`monitor_port` 或 `upload_port`)
- 波特率 (`monitor_speed`)

支持的平台：
1. **pico32** - ESP32原版开发板 ✅ 完全支持串口命令
2. **esp32-s3-devkitc-1** - ESP32-S3发布版（继承自esp32s3_common）❌ 暂不支持串口命令
3. **esp32-s3-debug** - ESP32-S3调试版（继承自esp32s3_common）✅ 完全支持串口命令

> 📌 **平台选择建议**：
> - 开发和调试阶段：推荐使用 `pico32` 或 `esp32-s3-debug`
> - 生产部署：使用 `esp32-s3-devkitc-1`（发布版不包含串口命令功能以节省资源）

### 自定义端口

如果需要临时使用不同的端口，可以通过Python命令行参数指定：

```bash
cd cybird_watching_cli
uv run python -m cybird_watching_cli.main -p COM4 --platform esp32-s3-devkitc-1
```

或者直接修改 `platformio.ini` 中对应平台的配置。

## 🛠️ 故障排除

### 常见问题

1. **"找不到cybird_watching_cli目录"**
   - 确保bat文件位于scripts目录中
   - 检查cybird_watching_cli文件夹是否存在于scripts目录下

2. **"未找到uv包管理器"**
   - 安装uv: https://docs.astral.sh/uv/
   - 确保uv已添加到系统PATH
   - 验证安装: 在命令行运行 `uv --version`

3. **设备连接失败**
   - 检查设备是否正确连接到电脑
   - 确认端口号（在设备管理器中查看COM端口）
   - 检查设备是否被其他程序（如Arduino IDE、PlatformIO Monitor）占用
   - 尝试使用 `reconnect` 命令重新连接

4. **中文显示乱码**
   - bat文件已设置UTF-8编码 (`chcp 65001`)
   - 如果仍有问题，可能是终端字体不支持中文
   - 建议使用Windows Terminal或其他支持UTF-8的终端

5. **文件上传/下载失败**
   - 确保设备端文件系统已正确初始化
   - 检查路径格式：远程路径必须以 `/` 开头
   - 验证设备端有足够的存储空间
   - 对于包含空格的路径，确保正确使用引号

6. **平台选择错误**
   - 选择平台时输入1-3之间的数字
   - 如果选择无效，将默认使用pico32平台
   - 可以通过Python命令行参数直接指定平台

7. **ESP32-S3发布版无响应**
   - ESP32-S3发布版（esp32-s3-devkitc-1）不支持串口命令
   - 如需使用CLI工具，请选择 `esp32-s3-debug` (调试版)
   - 或者使用 `pico32` (ESP32原版) 进行开发调试


### 调试模式

如果遇到问题，可以：

1. **使用交互模式逐步测试**：
   ```bash
   cybird_cli.bat
   # 在交互模式中：
   [OFF] CybirdWatching> reconnect    # 尝试重新连接
   [ON] CybirdWatching> test          # 测试连接
   [ON] CybirdWatching> info          # 查看连接信息
   ```

2. **先测试基本通信**：
   ```bash
   cd cybird_watching_cli
   uv run python -m cybird_watching_cli.main -p COM3 send "help"
   ```

3. **查看设备信息**：
   ```bash
   # 在交互模式中
   [ON] CybirdWatching> status
   [ON] CybirdWatching> info
   ```

4. **检查文件系统**：
   ```bash
   [ON] CybirdWatching> file list /
   [ON] CybirdWatching> file list /configs
   ```

## 💡 使用技巧

1. **创建桌面快捷方式**
   - 右键 `cybird_cli.bat` → 发送到桌面快捷方式
   - 双击桌面快捷方式即可快速启动
   - 可以为不同平台创建不同的快捷方式

2. **命令历史导航**
   - 使用 ↑ 和 ↓ 键浏览历史命令
   - CLI会自动保存命令历史
   - 支持跨会话的历史记录

3. **Tab补全**（部分支持）
   - 在某些终端中可以使用Tab键自动补全命令
   - 依赖于终端的支持程度

4. **批量操作**
   - 可以编写脚本文件，包含多个命令
   - 使用Python的send子命令逐行执行

5. **文件路径处理**
   - 支持相对路径（如 `./config.json`）
   - 支持绝对路径（如 `C:/Users/xxx/config.json`）
   - 远程路径必须以 `/` 开头（如 `/configs/bird_config.json`）
   - 路径包含空格时会自动处理

## 📝 高级用例

### 文件管理流程

```bash
# 1. 查看设备文件系统
[ON] CybirdWatching> file list /
[ON] CybirdWatching> file list /configs

# 2. 备份配置文件
[ON] CybirdWatching> download /configs/bird_config.json ./backup/bird_config_$(date).json

# 3. 修改配置后上传
[ON] CybirdWatching> upload ./modified_config.json /configs/bird_config.json

# 4. 验证配置
[ON] CybirdWatching> bird list
[ON] CybirdWatching> bird stats
```

### 日常开发流程

```bash
# 1. 启动CLI
cybird_cli.bat
# 选择: 3 (esp32-s3-debug) - 注意：必须选择调试版才支持串口命令

# 2. 检查设备状态
[ON] CybirdWatching> status
[ON] CybirdWatching> log lines 20

# 3. 测试观鸟功能
[ON] CybirdWatching> bird trigger
[ON] CybirdWatching> bird stats

# 4. 查看日志
[ON] CybirdWatching> log lines 50

# 5. 需要时更新配置
[ON] CybirdWatching> upload ./new_config.json /configs/bird_config.json

# 6. 退出
[ON] CybirdWatching> quit
```

### 多设备管理

```bash
# 设备1 (ESP32, COM3)
cd cybird_watching_cli
uv run python -m cybird_watching_cli.main -p COM3 --platform pico32

# 设备2 (ESP32-S3, COM4)  
cd cybird_watching_cli
uv run python -m cybird_watching_cli.main -p COM4 --platform esp32-s3-devkitc-1
```

## 🔧 技术细节

### CLI架构

```
cybird_watching_cli/
├── src/cybird_watching_cli/
│   ├── main.py              # 主入口，参数解析
│   ├── config/              # 配置管理
│   │   └── settings.py      # 配置类定义
│   ├── core/                # 核心功能
│   │   ├── connection.py    # 串口连接管理
│   │   ├── command_executor.py  # 命令执行器
│   │   ├── response_handler.py  # 响应处理器
│   │   └── file_transfer.py     # 文件传输
│   ├── ui/                  # 用户界面
│   │   └── console.py       # 控制台界面
│   └── utils/               # 工具函数
│       └── exceptions.py    # 异常定义
```

### 支持的命令格式

**本地命令**（在CLI内处理）：
- `help`, `quit`, `exit`, `cls`, `info`, `reconnect`, `test`, `reset`
- `upload <本地> <远程>` / `file upload <本地> <远程>`
- `download <远程> <本地>` / `file download <远程> <本地>`

**设备命令**（发送到ESP32）：
- `status`, `help`, `log [lines <n>]`, `log clear`
- `bird trigger [id]`, `bird stats`, `bird list`, `bird reset`
- `file list <路径>`
- 其他设备端实现的命令

### 响应处理机制

CLI使用特殊标记来解析设备响应：
- `<<<CMD_START>>>` - 命令响应开始
- `<<<CMD_END>>>` - 命令响应结束
- `<<<CMD_OK>>>` - 命令成功
- `<<<CMD_ERROR>>>` - 命令失败

这确保了即使有异步日志输出，也能正确接收命令响应。

---

这些工具让您可以更方便地使用CybirdWatching CLI，支持ESP32和ESP32-S3多平台开发！