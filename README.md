# 🐦 CybirdWatching

一个基于 ESP32 的智能观鸟显示设备，采用双核 FreeRTOS 架构，集成了多种传感器和动画系统。

当前项目的所有基础来自于[稚晖君的HoloCubic项目](https://github.com/peng-zhihui/HoloCubic)。

![Version](https://img.shields.io/badge/version-3.0.0-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32%20PICO32-green.svg)
![Framework](https://img.shields.io/badge/framework-Arduino-red.svg)
![License](https://img.shields.io/badge/license-MIT-yellow.svg)
![Build](https://img.shields.io/badge/build-PlatformIO-orange.svg)

**资源说明**
本项目中附带的所有动画资源为网络资源通过 AI 生成的视频，如有侵权，请联系删除。


## 📑 目录

- [✨ 特性](#-特性)
- [🛠️ 硬件要求](#️-硬件要求)
- [📦 软件依赖](#-软件依赖)
- [🚀 快速开始](#-快速开始)
- [📖 使用说明](#-使用说明)
  - [串口命令](#串口命令)
  - [IMU 手势控制](#imu-手势控制)
- [📁 项目结构](#-项目结构)
- [🏗️ 架构设计](#️-架构设计)
- [🔧 开发指南](#-开发指南)
- [🐛 故障排查](#-故障排查)
- [📝 更新日志](#-更新日志)
- [📚 相关文档](#-相关文档)
- [🤝 贡献](#-贡献)
- [📄 许可证](#-许可证)
- [🙏 致谢](#-致谢)

## ✨ 特性

### 🚀 双核架构
- **Core 0**: 专用 UI 渲染任务 (200Hz)
  - LVGL 图形界面系统
  - TFT 显示驱动
  - 流畅的小鸟动画播放
  
- **Core 1**: 系统逻辑任务 (100Hz)
  - IMU 传感器数据采集
  - 串口命令处理
  - WiFi 网络通信
  - 业务逻辑处理

### 🎨 核心功能
- 📺 **高性能图形界面** - 基于 LVGL 的流畅 UI 系统
- 🐦 **小鸟动画系统** - 支持多种小鸟动画，可按权重随机触发
- 🎮 **IMU 手势控制** - MPU6050 六轴传感器作为输入设备
- 💾 **SD 卡存储** - 配置文件、日志文件、资源存储
- 🌈 **RGB LED 指示** - FastLED 驱动的状态指示灯
- 📊 **统计系统** - 观鸟记录和统计分析，支持界面查看
- ⌨️ **串口命令系统** - 丰富的调试和控制命令
- 🔧 **任务监控系统** - 实时监控双核任务状态
- 📁 **文件管理** - 支持文件上传、下载、目录树查看

## 🛠️ 硬件要求

- **主控**: ESP32 (PICO32)
- **显示**: TFT LCD (240x240, TFT_eSPI 兼容)
- **传感器**: MPU6050 六轴传感器 (IMU)
- **存储**: Micro SD 卡 (建议 FAT32 格式)
- **LED**: WS2812B RGB LED
- **接口**: USB 串口 (默认 115200 波特率)

## 📦 软件依赖

### 核心库
- **Arduino Framework** - ESP32 框架
- **LVGL** - 轻量级图形库
- **TFT_eSPI** - 高性能 TFT 驱动
- **FastLED** - RGB LED 控制库
- **MPU6050** - IMU 传感器驱动
- **SPI/SD** - SD 卡存储驱动

### 开发工具
- **PlatformIO** - 项目构建和管理
- **ESP32 Arduino Core** - ESP32 支持包
- **Python 3.8+** - 脚本工具运行环境
- **uv** - Python 包管理器（用于 CLI 工具）

## 🚀 快速开始

### 1. 克隆项目
```bash
git clone <repository-url>
cd cybird-watching
```

### 2. 配置 PlatformIO
确保已安装 [PlatformIO IDE](https://platformio.org/install) 或 PlatformIO Core。

推荐用 Python 安装，本项目的 scripts 全部基于 Python 编写，后面也是会用到的。
```bash
pip install -U platformio
```

### 3. 准备 SD 卡
将 SD 卡插入电脑，确保 SD 卡已格式化为 FAT32 格式。
将 `resources/` 目录下的所有内容拷贝到 SD 卡根目录，不需要包含 `resources/` 目录本身。

SD 卡内容结构：
```
SD卡根目录/
├── birds/              # 小鸟图片资源
│   ├── 1001/          # 小鸟 ID 目录
│   ├── 1002/
│   └── ...
├── configs/           # 配置文件
│   └── bird_config.csv
└── static/            # 静态资源
    └── logo.bin       # 启动 Logo
```

### 4. 串口配置修改
本项目默认使用串口 **COM5**，波特率 **115200**。

如需修改，编辑 `platformio.ini`：
```ini
monitor_speed = 115200
upload_port = COM5      # 修改为你的端口
```


### 5. 编译和上传
```bash
# 编译项目
platformio run

# 上传到设备
platformio run --target upload

# 监控串口输出
platformio device monitor
```

也可以使用 `scripts/` 下的脚本（Windows）：
```bash
cd scripts

# 编译项目
.\pio_run.bat

# 上传固件并监控
.\upload_and_monitor.bat
```

### 6. 首次启动
设备启动后会自动：
1. 初始化所有硬件模块
2. 加载 SD 卡配置
3. 创建双核 FreeRTOS 任务
4. 启动 LVGL 图形界面

## 📖 使用说明

### 串口命令

运行 `scripts/` 下的脚本：
```pwsh
cd scripts/
.\cybird_cli.bat
```

连接串口（115200 波特率）后，可使用以下命令：

#### 系统控制
```bash
help                # 显示所有可用命令
status              # 显示系统状态
reboot              # 重启设备（未实现）
clear               # 清空终端屏幕
```

#### 小鸟系统
```bash
bird trigger [id]   # 触发小鸟动画（可选指定小鸟ID，如 bird trigger 1001）
bird list           # 列出所有小鸟
bird stats          # 查看观鸟统计
bird reset          # 重置统计数据
```

#### 日志管理
```bash
log                 # 查看日志内容（默认最后 20 行）
log lines <N>       # 查看最后 N 行日志
log cat             # 查看完整日志内容
log clear           # 清空日志文件
log size            # 查看日志文件大小
log level <level>   # 设置日志级别 (DEBUG/INFO/WARN/ERROR)
```

#### 文件管理
```bash
tree [path] [levels]    # 显示 SD 卡目录树（默认根目录，2 层）
file upload <path>      # 上传文件（需要 CLI 工具）
file download <path>    # 下载文件（需要 CLI 工具）
file delete <path>      # 删除文件
file info <path>        # 查看文件信息
```

大文件还是建议直接插 SD 卡操作。


CLI 工具特性：
- 🎯 交互式命令行界面
- 📝 命令历史和自动补全
- 🎨 彩色输出
- 📁 文件上传/下载支持
- 🔄 自动重连

详见：[CLI 工具使用指南](scripts/README_CLI_TOOLS.md)

### IMU 手势控制
- **切换小鸟**：在小鸟展示界面，保持左倾/右倾可以触发切换小鸟（CD 为 10 秒），触发成功 LED 闪烁蓝灯
- **统计界面**：保持前倾进入统计界面，触发成功 LED 闪烁绿光。在统计界面内，保持左倾/右倾进行翻页
- **退出统计**：在统计界面保持后倾可退出到小鸟界面

## 📁 项目结构

```
cybird-watching/
├── src/                                  # 源代码
│   ├── main.cpp                          # 主程序入口（双核任务初始化）
│   ├── config/                           # 配置文件
│   │   └── guider_fonts.h                # LVGL 字体配置
│   ├── drivers/                          # 硬件驱动层
│   │   ├── display/                      # 显示驱动 (TFT_eSPI)
│   │   ├── sensors/                      # 传感器驱动
│   │   │   ├── imu/                      # IMU (MPU6050)
│   │   │   └── ambient/                  # 环境传感器
│   │   ├── communication/network/        # WiFi 网络
│   │   ├── storage/sd_card/              # SD 卡存储
│   │   └── io/rgb_led/                   # RGB LED (FastLED)
│   ├── system/                           # 系统服务层
│   │   ├── logging/                      # 日志管理系统
│   │   ├── commands/                     # 串口命令系统
│   │   ├── tasks/                        # 任务管理器 (v3.0 双核架构)
│   │   └── lvgl/ports/                   # LVGL 端口层
│   │       ├── lv_port_indev.c           # 输入设备端口
│   │       └── lv_port_fatfs.c           # 文件系统端口
│   └── applications/                     # 应用层
│       ├── gui/                          # 图形界面
│       │   ├── core/                     # GUI 核心
│       │   │   ├── lv_cubic_gui.cpp      # GUI 主逻辑
│       │   │   ├── gui_guider.c          # GUI 引导
│       │   │   └── events_init.c         # 事件初始化
│       │   └── screens/                  # 界面屏幕
│       │       ├── setup_scr_home.c      # 主屏幕
│       │       ├── setup_scr_scenes.c    # 场景屏幕
│       │       └── bird_animation_bridge.cpp  # 动画桥接
│       └── modules/                      # 功能模块
│           ├── bird_watching/            # 观鸟模块
│           │   ├── core/                 # 核心逻辑
│           │   │   ├── bird_animation.cpp    # 动画系统
│           │   │   ├── bird_manager.cpp      # 小鸟管理器
│           │   │   ├── bird_selector.cpp     # 小鸟选择器
│           │   │   ├── bird_stats.cpp        # 统计系统
│           │   │   ├── bird_utils.cpp        # 工具函数
│           │   │   ├── bird_types.h          # 类型定义
│           │   │   └── bird_watching.cpp     # 主模块
│           │   └── ui/                   # UI 组件
│           │       └── stats_view.cpp        # 统计视图
│           └── resources/                # 资源文件
│               ├── fonts/                # 嵌入字体
│               └── images/               # 嵌入图片
├── lib/                                  # 第三方库
│   └── [多个外部库]
├── resources/                            # SD 卡资源文件
│   ├── birds/                            # 小鸟图片资源（按 ID 分目录）
│   ├── configs/                          # 配置文件
│   │   └── bird_config.csv               # 小鸟配置
│   └── static/                           # 静态资源
│       └── logo.bin                      # 启动 Logo
├── scripts/                              # 工具脚本
│   ├── cybird_cli.bat                    # CLI 快捷启动
│   ├── upload_and_monitor.bat            # 上传监控脚本
│   ├── pio_run.bat                       # 编译脚本
│   ├── README_CLI_TOOLS.md               # CLI 工具说明
│   ├── cybird_watching_cli/              # Python CLI 工具
│   ├── converter/                        # 图片转换工具
│   ├── mp4converter/                     # 视频转换工具
│   └── uniq_fonts/                       # 字体工具
├── docs/                                 # 项目文档
│   ├── CHANGELOG_v3.0.md                 # v3.0 更新日志
│   ├── DUAL_CORE_ARCHITECTURE.md         # 双核架构说明
│   ├── BIRD_WATCHING_FLOW_DIAGRAM.md     # 流程图
│   ├── bird_watching_test_guide.md       # 测试指南
│   ├── STATS_VIEW_GUIDE.md               # 统计视图指南
│   ├── FILE_TRANSFER_GUIDE.md            # 文件传输指南
│   ├── CHANGE_FONT_SIZE.md               # 字体修改指南
│   └── LVGL_9X_UPGRADE_ANALYSIS.md       # LVGL 升级分析
├── platformio.ini                        # PlatformIO 配置
├── CLAUDE.md                             # 如果你也用 Claude Code，可以直接使用
├── LICENSE                               # MIT 许可证
└── README.md                             # 本文件
```

## 🏗️ 架构设计

### 双核 FreeRTOS 架构

```
┌─────────────────────────────────────────────────────┐
│                     ESP32                           │
├─────────────────────┬───────────────────────────────┤
│   Core 0 (UI)       │   Core 1 (System)             │
│   优先级: 2         │   优先级: 1                    │
│   栈: 8KB           │   栈: 8KB                      │
│   频率: 200Hz       │   频率: 100Hz                  │
├─────────────────────┼───────────────────────────────┤
│ • LVGL GUI          │ • IMU 传感器                   │
│ • Display 刷新      │ • 串口命令                     │
│ • 小鸟动画          │ • WiFi 通信                    │
│ • 图片解码          │ • SD 卡操作                    │
│                     │ • 业务逻辑                     │
└─────────────────────┴───────────────────────────────┘
           ↕                        ↕
    ┌──────────────────────────────────────┐
    │   消息队列 + LVGL 互斥锁              │
    └──────────────────────────────────────┘
```

## 🔧 开发指南

### 添加新的串口命令
编辑 `src/system/commands/serial_commands.cpp`：

```cpp
void SerialCommands::initialize() {
    // 注册你的命令
    registerCommand("mycommand", "My command description");
}

void SerialCommands::processCommand(const String& command, const String& args) {
    if (command == "mycommand") {
        Serial.println("Command executed!");
        // 你的逻辑
    }
}
```

### 访问 LVGL 对象（线程安全）
在 v3.0 双核架构中，所有跨任务访问 LVGL 对象都必须加锁：

```cpp
TaskManager* taskMgr = TaskManager::getInstance();

// 获取互斥锁（最多等待 100ms）
if (taskMgr->takeLVGLMutex(100)) {
    // 安全地访问 LVGL 对象
    lv_obj_set_pos(obj, x, y);
    lv_label_set_text(label, "Hello");
    
    // 释放互斥锁
    taskMgr->giveLVGLMutex();
} else {
    LOG_ERROR("TAG", "Failed to take LVGL mutex");
}
```

### 添加新的小鸟资源
参考 [添加小鸟指引](docs/GUIDE_TO_ADD_NEW_BIRDS.md)

### 使用日志系统
```cpp
#include "system/logging/log_manager.h"

// 不同级别的日志
LOG_DEBUG("TAG", "Debug message");
LOG_INFO("TAG", "Info message");
LOG_WARN("TAG", "Warning message");
LOG_ERROR("TAG", "Error message");

// 格式化日志
LOG_INFO("TAG", "Value: %d, String: %s", value, str);
```

## 🐛 故障排查

### UI 卡顿或动画不流畅
```bash
task stats          # 检查栈使用情况和 CPU 占用
task info           # 查看详细系统信息
```
**可能原因：**
- UI 任务栈溢出（检查剩余栈空间）
- LVGL 互斥锁死锁（检查是否有未释放的锁）
- SD 卡读取速度慢（使用高速卡）
- 考虑增加 UI 任务优先级或栈大小

### 串口命令无响应
**可能原因：**
- 波特率不匹配（确认为 115200）
- 系统任务崩溃（查看 `task stats`）
- 串口被其他程序占用
- 看门狗复位（检查日志）

**解决方法：**
```bash
# 检查任务状态
task stats

# 查看日志
log lines 50
```

### SD 卡读取失败
**可能原因：**
- SD 卡格式不是 FAT32
- SD 卡接触不良
- 文件路径错误
- SD 卡容量过大（建议 ≤32GB）

**解决方法：**
```bash
# 查看 SD 卡目录
tree / 2

# 查看日志中的错误
log cat

# 检查文件是否存在
file info /birds/1001/0.bin
```

### 小鸟动画不显示
**可能原因：**
- 图片文件缺失或损坏
- 配置文件错误
- 内存不足

**解决方法：**
```bash
# 列出所有小鸟
bird list

# 查看统计（确认资源是否加载）
bird stats

# 手动触发测试
bird trigger 1001

# 查看日志
log lines 100
```

### 任务栈溢出
**症状：**系统频繁重启，日志显示 "Stack overflow"

**解决方法：**
编辑 `src/system/tasks/task_manager.cpp`：
```cpp
// 增加栈大小
#define UI_TASK_STACK_SIZE      10240  // 改为 10KB
#define SYSTEM_TASK_STACK_SIZE  10240  // 改为 10KB
```

## 📚 相关文档

### 核心文档
- [双核架构详细说明](docs/DUAL_CORE_ARCHITECTURE.md) - FreeRTOS 双核设计
- [开发指南](CLAUDE.md) - AI 辅助开发说明

### 功能文档
- [观鸟系统流程图](docs/BIRD_WATCHING_FLOW_DIAGRAM.md) - 系统流程详解
- [测试指南](docs/bird_watching_test_guide.md) - 功能测试步骤
- [统计视图指南](docs/STATS_VIEW_GUIDE.md) - 统计界面使用

### 工具文档
- [CLI 工具使用](scripts/README_CLI_TOOLS.md) - 命令行工具指南
- [文件传输指南](docs/FILE_TRANSFER_GUIDE.md) - 文件上传下载
- [字体修改指南](docs/CHANGE_FONT_SIZE.md) - 自定义字体大小

### 技术分析
- [LVGL 9.x 升级分析](docs/LVGL_9X_UPGRADE_ANALYSIS.md) - 版本升级注意事项

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

### 贡献指南
1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

### 代码规范
- 使用清晰的注释（中文或英文）
- 遵循现有代码风格
- 添加必要的文档说明
- 测试新功能的稳定性

## 📄 许可证

本项目采用 **MIT 许可证**。详见 [LICENSE](LICENSE) 文件。

Copyright (c) 2025 Mango

## 🙏 致谢

- [HoloCubic](https://github.com/peng-zhihui/HoloCubic) - 项目灵感和基础框架
- [LVGL](https://lvgl.io/) - 图形库支持
- ESP32 社区 - 硬件和技术支持
- 懂鸟 - 鸟类知识科普平台
- 所有的观鸟爱好者 🐦💕

---

<div align="center">

**🐦 Happy Bird Watching! 🐦**

*让科技与自然相遇，用代码记录美好瞬间*

</div>
