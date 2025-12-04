# CybirdWatching CLI

跨平台的CybirdWatching ESP32设备命令行工具，提供与PowerShell版本相同的功能，支持Windows、Linux和macOS。

## 功能特性

- **跨平台支持**: Windows、Linux、macOS
- **异步串口通信**: 高效的设备通信
- **丰富的CLI体验**: 彩色输出、状态指示、命令提示
- **完整的命令支持**: 兼容所有ESP32设备命令
- **配置管理**: YAML配置文件支持
- **错误处理**: 完善的异常处理和重试机制

## 安装和设置

### 使用uv（推荐）

```bash
cd src/cybird_watching_cli
uv install
```

### 手动安装依赖

```bash
pip install pyserial click rich pyyaml
```

## 使用方法

### 交互式模式

```bash
# 启动交互式模式
uv run python -m cybird_watching_cli.main

# 或使用安装后的命令
cybird-cli
```

### 指定连接参数

```bash
# 指定端口
uv run python -m cybird_watching_cli.main --port COM4

# 指定波特率
uv run python -m cybird_watching_cli.main --baudrate 9600

# 同时指定端口和波特率
uv run python -m cybird_watching_cli.main --port /dev/ttyUSB0 --baudrate 115200
```

### 单命令模式

```bash
# 发送单个命令
uv run python -m cybird_watching_cli.main send "log"

# 查看设备状态
uv run python -m cybird_watching_cli.main send "status"

# 显示帮助
uv run python -m cybird_watching_cli.main send "help"
```

## 支持的命令

### 设备命令

#### 日志管理
| 命令 | 描述 |
|------|------|
| `log` | 显示最后20行日志 |
| `log clear` | 清空日志文件 |
| `log size` | 显示日志文件大小 |
| `log lines N` | 显示最后N行日志 (1-500) |
| `log cat` | 显示完整日志文件内容 |

#### 系统状态
| 命令 | 描述 |
|------|------|
| `status` | 显示系统状态 |
| `clear` | 清除设备终端屏幕 |
| `help` | 显示设备帮助 |

#### 文件管理
| 命令 | 描述 |
|------|------|
| `tree [path] [levels]` | 显示SD卡目录树 |

#### 观鸟功能 🐦
| 命令 | 描述 |
|------|------|
| `bird list` | 显示可用小鸟列表 |
| `bird trigger` | 手动触发小鸟动画 |
| `bird stats` | 显示观鸟统计信息 |
| `bird status` | 显示观鸟系统状态 |
| `bird reset` | 重置观鸟统计数据 |
| `bird help` | 显示观鸟命令帮助 |

### 本地命令

| 命令 | 描述 |
|------|------|
| `help` | 显示CLI帮助 |
| `test` | 测试基本通信（无响应标记） |
| `reset` | 重置观鸟统计数据并立即落盘 |
| `quit`, `exit` | 退出程序 |
| `reconnect` | 重新连接设备 |
| `cls` | 清除终端屏幕 |
| `info` | 显示设备连接信息 |

## 使用示例

### 交互式使用

```bash
$ cybird-cli

欢迎使用 CybirdWatching CLI - 跨平台命令行工具 v1.0
===============================================
✓ 设备状态: ON (端口: COM3)

[ON] CybirdWatching> help
=== CybirdWatching CLI 帮助 ===
...

[ON] CybirdWatching> status
发送: status
响应:
=== CybirdWatching System Status ===
Log Manager: OK
SD Card: Available
Free Heap: 234512 bytes
Uptime: 120 seconds
...

[ON] CybirdWatching> log lines 50
发送: log lines 50
响应:
[INFO] 系统启动完成
[INFO] WiFi连接成功
[INFO] 传感器初始化完成
...

[ON] CybirdWatching> bird list
发送: bird list
响应:
=== Available Birds ===
ID     Name              Weight   Frames
----   --------------   ------   ------
1001   普通翠鸟          50       ?
1002   叉尾太阳鸟        30       ?
----   --------------   ------   ------
Total: 2 birds, Total Weight: 80
=== End of List ===

[ON] CybirdWatching> bird status
发送: bird status
响应:
=== Bird Watching System Status ===
Bird Manager: Initialized
Animation System: Idle
Statistics Records: 0
=== End Status ===

[ON] CybirdWatching> bird trigger
发送: bird trigger
响应:
Triggering bird appearance...
Bird triggered successfully!

[ON] CybirdWatching> quit
```

### 脚本使用

```bash
#!/bin/bash

# 检查设备状态
cybird-cli send "status"

# 获取最新日志
cybird-cli send "log lines 100" > device.log

# 查看观鸟统计
cybird-cli send "bird stats"

# 手动触发小鸟
cybird-cli send "bird trigger"

# 清空日志
cybird-cli send "log clear"
```

## 配置

### 配置文件

程序支持通过YAML配置文件进行配置：

- `config/default.yaml`: 默认配置
- `config/user.yaml`: 用户自定义配置（覆盖默认配置）

### 配置项

```yaml
# 串口连接配置
serial:
  port: "COM3"           # 串口端口
  baudrate: 115200       # 波特率
  timeout: 3.0          # 连接超时(秒)
  write_timeout: 3.0    # 写入超时(秒)

# 响应处理配置
response:
  response_start_marker: "<<<RESPONSE_START>>>"
  response_end_marker: "<<<RESPONSE_END>>>"
  command_timeout_ms: 6000      # 命令超时(毫秒)
  response_wait_ms: 300         # 响应等待时间(毫秒)
  data_read_interval_ms: 50     # 数据读取间隔(毫秒)
  no_data_timeout_ms: 800       # 无数据超时(毫秒)

# 用户界面配置
ui:
  enable_colors: true           # 启用彩色输出
  prompt_template: "{status} CybirdWatching> "  # 命令提示符模板

# 日志配置
log_level: "INFO"              # 日志级别
```

## 开发

### 项目结构

```
src/cybird_watching_cli/
├── cybird_cli/              # 主包
│   ├── main.py              # 主入口点
│   ├── config/              # 配置管理
│   ├── core/                # 核心模块
│   │   ├── connection.py    # 串口连接
│   │   ├── response_handler.py  # 响应处理
│   │   └── command_executor.py  # 命令执行
│   ├── ui/                  # 用户界面
│   │   └── console.py       # 控制台界面
│   └── utils/               # 工具模块
│       └── exceptions.py    # 自定义异常
├── config/                  # 配置文件
└── pyproject.toml          # 项目配置
```

### 运行测试

```bash
# 运行主程序
uv run python -m cybird_watching_cli.main

# 使用开发模式
uv run python -m cybird_watching_cli.main --port COM3
```

## 故障排除

### 常见问题

1. **设备连接失败**
   - 检查串口端口是否正确
   - 确认设备已连接并 powered on
   - 检查端口是否被其他程序占用

2. **命令执行超时**
   - 检查设备是否正常响应
   - 尝试使用 `test` 命令测试基本通信
   - 检查波特率设置是否正确

3. **权限问题**（Linux/macOS）
   - 确保用户有串口设备访问权限
   - 可能需要将用户添加到 `dialout` 组

### 调试模式

使用 `test` 命令进行基本通信测试：

```bash
[ON] CybirdWatching> test
测试模式：发送简单ping命令（无响应标记）...
发送: help
响应:
=== Available Commands ===
  help             - Show available commands
  log              - Log file operations
  status           - Show system status
  clear            - Clear terminal screen
  tree             - Show SD card directory tree
```

## 许可证

本项目采用MIT许可证。

## 贡献

欢迎提交Issue和Pull Request来改进这个项目。

## 更新日志

### v1.1.0
- ✨ 新增 `bird list` 命令：显示可用小鸟列表
- ✨ 新增 `bird status` 命令：检查观鸟系统初始化状态
- 🔧 改进 `bird stats` 命令：显示更详细的统计信息
- 📚 更新帮助文档和示例

### v1.0.0
- 初始版本
- 支持所有基本的设备命令
- 跨平台串口通信
- Rich美化界面
- YAML配置支持