# 🐦 CybirdWatching

一个基于 ESP32/ESP32-S3 的智能观鸟显示设备，采用双核 FreeRTOS 架构，集成了硬件抽象层 (HAL)、多种传感器和流畅的动画系统。

![11](https://github.com/user-attachments/assets/90f8604d-6d39-4414-a31e-e3bdec304df7)

当前项目的所有基础来自于[稚晖君的HoloCubic项目](https://github.com/peng-zhihui/HoloCubic)。

> 淘宝上搜索关键字“HoloCubic”、“稚晖君”就能找到很多出售整机的商家，各家都大差不差（我也是直接购买的硬件）。购买预装了[HoloCubic_AIO](https://github.com/ClimbSnail/HoloCubic_AIO)固件的版本即可。

![Version](https://img.shields.io/badge/version-1.1.0-blue.svg)
![Platform](https://img.shields.io/badge/platform-ESP32%20|%20ESP32--S3-green.svg)
![Framework](https://img.shields.io/badge/framework-Arduino-red.svg)
![License](https://img.shields.io/badge/license-MIT-yellow.svg)
![Build](https://img.shields.io/badge/build-PlatformIO-orange.svg)
![Architecture](https://img.shields.io/badge/architecture-Dual--Core%20FreeRTOS-purple.svg)

**资源说明**

本项目中附带的所有动画资源为网络资源通过 AI 生成的视频，如有侵权，请联系删除。


## 🎮 操控说明

### IMU 手势控制
- **切换小鸟**：在小鸟展示界面，保持左倾/右倾可以触发切换小鸟（CD 为 10 秒），触发成功 LED 闪烁蓝灯
- **进入统计**：保持前倾进入统计界面，触发成功 LED 闪烁绿光
- **统计翻页**：在统计界面内，保持左倾/右倾进行翻页
- **退出统计**：在统计界面保持后倾可退出到小鸟界面

### RGB LED 状态指示
- 🔵 **蓝灯闪烁**：系统启动 / 切换小鸟成功
- 🟢 **绿灯闪烁**：进入统计界面 / IMU 数据有效
- 🔴 **红灯闪烁**：硬件初始化失败
- 🟡 **黄灯闪烁**：传感器数据异常

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
  - TFT 显示驱动 (ST7789)
  - 流畅的小鸟动画播放
  - 图片解码和渲染
  
- **Core 1**: 系统逻辑任务 (100Hz)
  - IMU 传感器数据采集 (5Hz)
  - 串口命令处理
  - WiFi 网络通信
  - SD 卡文件操作
  - 业务逻辑处理
  - 统计数据管理

### 🎨 核心功能
- 📺 **高性能图形界面** - 基于 LVGL 9.4 的流畅 UI 系统
- 🐦 **智能观鸟系统** - 支持多种小鸟动画，可按权重随机触发
- 🎮 **IMU 手势控制** - MPU6050/QMI8658 六轴传感器作为输入设备
- 💾 **SD 卡存储** - 配置文件、日志文件、资源存储 (支持 SPI/SDMMC 模式)
- 🌈 **RGB LED 指示** - FastLED 驱动的状态指示灯
- 📊 **统计系统** - 观鸟记录和统计分析，支持界面查看和翻页
- ⌨️ **串口命令系统** - 丰富的调试和控制命令
- 🔧 **任务监控系统** - 实时监控双核任务状态和性能
- 📁 **文件管理** - 支持文件上传、下载、目录树查看
- 🛡️ **硬件抽象层 (HAL)** - 跨平台硬件适配，自动检测传感器型号

## 🛠️ 硬件要求

### 支持的主控平台

| 平台 | 开发板 | SD 卡模式 | IMU 传感器 |
|------|--------|----------|-----------|
| **ESP32** | PICO32 (HoloCubic) | SPI | MPU6050 |
| **ESP32-S3** | DevKitC-1 | SDMMC / SPI | MPU6050 / QMI8658 |

### 通用硬件
- **显示**: TFT LCD (240x240, ST7789) - 通过 LovyanGFX 驱动
- **存储**: Micro SD 卡 (建议 FAT32 格式，≤32GB)
  - ESP32: SPI 模式
  - ESP32-S3: SDMMC 模式（优先，最高 40MHz）或 SPI 模式（回退）
- **LED**: WS2812B RGB LED
- **接口**: USB 串口 (默认 115200 波特率)
  - ESP32: 传统 UART
  - ESP32-S3: USB CDC (调试版) 或传统 UART (发布版)

### ESP32-S3 引脚映射

<details>
<summary>点击展开引脚对照表</summary>

| 功能 | ESP32 引脚 | ESP32-S3 引脚 |
|------|-----------|--------------|
| **TFT SPI** | | |
| MISO | GPIO19 | GPIO13 |
| MOSI | GPIO23 | GPIO11 |
| SCLK | GPIO18 | GPIO12 |
| DC | GPIO2 | GPIO10 |
| RST | GPIO4 | GPIO9 |
| BL | GPIO5 | GPIO8 |
| **SD 卡 (SPI)** | | |
| SCK | GPIO14 | GPIO14 |
| MISO | GPIO26 | GPIO15 |
| MOSI | GPIO13 | GPIO16 |
| CS | GPIO15 | GPIO17 |
| **SD 卡 (SDMMC)** | 不支持 | CLK=36, CMD=35, D0=37 |
| **I2C (IMU)** | SDA=32, SCL=33 | SDA=1, SCL=2 |
| **RGB LED** | GPIO27 | GPIO48 |

</details>

## 📦 软件依赖

### 核心库
- **Arduino Framework** - ESP32/ESP32-S3 框架
- **LVGL 9.4** - 轻量级图形库
- **LovyanGFX** - 高性能 TFT 驱动 (替代 TFT_eSPI)
- **FastLED** - RGB LED 控制库
- **MPU6050_light / QMI8658** - IMU 传感器驱动
- **SPI/SD/SD_MMC** - SD 卡存储驱动

### 技术栈
```
┌─────────────────────────────────────────────┐
│          应用层 (Application)               │
│  - Bird Watching System                    │
│  - GUI Manager (LVGL 9.4)                  │
│  - Statistics & Logging                    │
├─────────────────────────────────────────────┤
│          系统层 (System)                    │
│  - Task Manager (FreeRTOS)                 │
│  - Command System                          │
│  - Log Manager                             │
├─────────────────────────────────────────────┤
│      硬件抽象层 (HAL)                        │
│  - IMU Interface (MPU6050/QMI8658)         │
│  - SD Interface (SDMMC/SPI)                │
│  - Hardware Detector                       │
├─────────────────────────────────────────────┤
│         驱动层 (Drivers)                    │
│  - Display (LovyanGFX)                     │
│  - IMU (MPU6050/QMI8658)                   │
│  - RGB LED (FastLED)                       │
│  - SD Card (SPI/SDMMC)                     │
├─────────────────────────────────────────────┤
│      硬件层 (Hardware)                      │
│  ESP32 / ESP32-S3                          │
└─────────────────────────────────────────────┘
```

### 开发工具
- **PlatformIO** - 项目构建和管理
- **ESP32 Arduino Core** - ESP32/ESP32-S3 支持包
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
本项目默认串口配置：
- **ESP32 (pico32)**: COM3，波特率 115200
- **ESP32-S3 (调试版)**: COM4，波特率 115200，USB CDC 模式
- **ESP32-S3 (发布版)**: COM4，波特率 115200，传统 UART 模式

如需修改，编辑 `platformio.ini`：
```ini
[env:pico32]
monitor_speed = 115200
upload_port = COM3      # ESP32 端口

[env:esp32-s3-debug]
monitor_speed = 115200
upload_port = COM4      # ESP32-S3 端口
monitor_port = COM4
```


### 5. 编译和上传

#### 选择目标平台

项目支持两种硬件平台，编译时需要指定环境：

```bash
# ESP32 PICO32 (HoloCubic 默认)
platformio run -e pico32
platformio run -e pico32 --target upload

# ESP32-S3 DevKitC-1 (调试版，启用 USB CDC)
platformio run -e esp32-s3-debug
platformio run -e esp32-s3-debug --target upload

# ESP32-S3 DevKitC-1 (发布版，独立运行)
platformio run -e esp32-s3-devkitc-1
platformio run -e esp32-s3-devkitc-1 --target upload

# 监控串口输出
platformio device monitor
```

#### 编译环境说明

| 环境名称 | 平台 | USB CDC | 适用场景 |
|---------|------|---------|---------|
| `pico32` | ESP32 | N/A | HoloCubic 开发板 |
| `esp32-s3-debug` | ESP32-S3 | 启用 | 开发调试（需要串口监视器） |
| `esp32-s3-devkitc-1` | ESP32-S3 | 禁用 | 正常使用（断电重启正常工作） |

也可以使用 `scripts/` 下的脚本（Windows）：
```bash
cd scripts

# 编译项目（会提示选择平台）
.\pio_run.bat

# 上传固件并监控
.\upload_and_monitor.bat
```

### 6. 首次启动
设备启动后会自动：
1. **早期 RGB LED 初始化** - 蓝灯快闪表示启动
2. **串口和日志系统初始化** - 显示平台信息和硬件配置
3. **硬件抽象层 (HAL) 初始化**
   - 自动检测 IMU 传感器型号 (MPU6050/QMI8658)
   - 测试传感器数据读取
   - RGB LED 状态反馈（绿灯=成功，红灯=失败，黄灯=异常）
4. **IMU 驱动层初始化** - 供 TaskManager 使用
5. **SD 卡初始化**
   - ESP32-S3: 优先尝试 SDMMC 模式，失败则回退到 SPI
   - ESP32: 使用 SPI 模式
   - 多频率自适应 (40MHz → 25MHz → 10MHz → 4MHz → 1MHz)
6. **显示屏和 LVGL 初始化** - 显示启动 Logo
7. **任务管理器初始化** - 创建双核 FreeRTOS 任务
8. **观鸟系统初始化** - 扫描 SD 卡资源，加载小鸟配置
9. **RGB LED 测试序列** - 运行彩虹效果测试

启动过程中，可通过 RGB LED 观察各阶段状态：
- 🔵 系统启动
- 🟢 硬件初始化成功
- 🔴 初始化失败
- 🟡 数据异常
- 🌈 启动完成（彩虹效果）

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
status              # 显示系统状态（平台、固件版本、内存使用）
reboot              # 重启设备
clear               # 清空终端屏幕
task stats          # 显示双核任务统计信息
task info           # 显示详细系统信息
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

## 📁 项目结构

```
cybird-watching/
├── src/                                  # 源代码
│   ├── main.cpp                          # 主程序入口（双核任务初始化）
│   ├── config/                           # 配置文件
│   │   ├── hardware_config.h             # 硬件配置（双平台引脚映射）
│   │   ├── version.h                     # 版本管理
│   │   └── guider_fonts.h                # LVGL 字体配置
│   ├── hal/                              # 硬件抽象层（跨平台适配）
│   │   ├── hal_manager.h/cpp             # HAL 管理器
│   │   ├── hardware_detector.h/cpp       # 硬件检测器
│   │   ├── imu_interface.h               # IMU 接口定义
│   │   ├── mpu6050_impl.h/cpp            # MPU6050 实现
│   │   ├── qmi8658_impl.h/cpp            # QMI8658 实现
│   │   ├── imu_factory.cpp               # IMU 工厂
│   │   └── sd_interface.h/cpp            # SD 卡接口（SPI/SDMMC）
│   ├── drivers/                          # 硬件驱动层
│   │   ├── display/                      # 显示驱动 (LovyanGFX)
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
│           │   │   ├── bird_bundle_loader.cpp# Bundle 加载器
│           │   │   ├── bird_types.h          # 类型定义
│           │   │   └── bird_watching.cpp     # 主模块
│           │   └── ui/                   # UI 组件
│           │       └── stats_view.cpp        # 统计视图
│           └── resources/                # 资源文件
│               ├── fonts/                # 嵌入字体
│               └── images/               # 嵌入图片
├── lib/                                  # 第三方库
│   ├── lvgl/                             # LVGL 9.4 图形库
│   ├── LovyanGFX/                        # 高性能 TFT 驱动
│   ├── FastLED/                          # RGB LED 库
│   ├── MPU6050_light/                    # MPU6050 驱动
│   └── [其他外部库]
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
│   ├── DUAL_CORE_ARCHITECTURE.md         # 双核架构说明
│   ├── BIRD_WATCHING_FLOW_DIAGRAM.md     # 流程图
│   ├── bird_watching_test_guide.md       # 测试指南
│   ├── STATS_VIEW_GUIDE.md               # 统计视图指南
│   ├── FILE_TRANSFER_GUIDE.md            # 文件传输指南
│   ├── CHANGE_FONT_SIZE.md               # 字体修改指南
│   └── LVGL_9X_UPGRADE_ANALYSIS.md       # LVGL 升级分析
├── platformio.ini                        # PlatformIO 配置
├── CLAUDE.md                             # Claude Code 项目说明
├── CHANGELOG.md                          # 更新日志
├── LICENSE                               # MIT 许可证
└── README.md                             # 本文件
```

## 🏗️ 架构设计

### 双核 FreeRTOS 架构

```
┌─────────────────────────────────────────────────────┐
│              ESP32 / ESP32-S3                       │
├─────────────────────┬───────────────────────────────┤
│   Core 0 (UI)       │   Core 1 (System)             │
│   优先级: 2         │   优先级: 1                    │
│   栈: 8KB           │   栈: 8KB                      │
│   频率: 200Hz       │   频率: 100Hz                  │
├─────────────────────┼───────────────────────────────┤
│ • LVGL GUI          │ • IMU 传感器 (5Hz)             │
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

### 硬件抽象层 (HAL)

```
┌─────────────────────────────────────────────────────┐
│                   Application Layer                 │
├─────────────────────────────────────────────────────┤
│                  HAL Manager                        │
│         (硬件检测 & 实例管理)                         │
├──────────────┬──────────────────┬───────────────────┤
│  IMU接口     │  SD接口          │  未来扩展...       │
├──────────────┼──────────────────┼───────────────────┤
│ MPU6050实现  │  SDMMC实现       │                   │
│ QMI8658实现  │  SPI实现         │                   │
└──────────────┴──────────────────┴───────────────────┘
           ↕            ↕
    ┌──────────────────────────────────────┐
    │        硬件层 (I2C/SPI/SDMMC)        │
    └──────────────────────────────────────┘
```

**HAL 特性**:
- 自动硬件检测和适配
- 统一的接口抽象
- 运行时传感器识别
- 多模式回退机制 (SDMMC → SPI)

详见：[双核架构详细说明](docs/DUAL_CORE_ARCHITECTURE.md)

## 🔧 开发指南

### 跨平台开发 (ESP32 / ESP32-S3)

本项目使用硬件抽象层 (HAL) 实现跨平台支持，主要通过以下方式：

#### 1. 编译时平台识别
```cpp
// 在 hardware_config.h 中定义
#ifdef PLATFORM_ESP32_S3
    // ESP32-S3 专用代码
#else
    // ESP32 专用代码
#endif
```

#### 2. 运行时硬件检测
```cpp
// HAL 自动检测传感器型号
HAL::HALManager& hal = HAL::HALManager::getInstance();
hal.initialize();  // 自动检测 MPU6050 或 QMI8658

// 获取 IMU 实例
HAL::IMUInterface* imu = hal.getIMU();
if (imu) {
    imu->update(10);
    int16_t ax = imu->getAccelX();
}
```

#### 3. 多模式回退
```cpp
// SD 卡初始化示例（自动 SDMMC → SPI 回退）
HAL::SDInterface::init();  // 内部自动尝试多种模式
```

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

### 添加新的传感器支持

1. **定义接口**（如果需要新接口类型）：
```cpp
// 在 hal/ 目录创建新接口头文件
class NewSensorInterface {
public:
    virtual void init() = 0;
    virtual int16_t readData() = 0;
};
```

2. **实现具体传感器**：
```cpp
// 继承接口并实现
class MySensorImpl : public NewSensorInterface {
    void init() override { /* ... */ }
    int16_t readData() override { /* ... */ }
};
```

3. **在 HAL Manager 中注册**：
```cpp
// 在 hal_manager.cpp 中添加检测逻辑
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

### 平台特定代码编写建议

1. **优先使用 HAL 接口**，避免直接访问硬件
2. **必要时使用条件编译**：
   ```cpp
   #ifdef PLATFORM_ESP32_S3
       // ESP32-S3 专用
   #else
       // ESP32 专用
   #endif
   ```
3. **在 `hardware_config.h` 中集中管理引脚配置**
4. **使用编译时断言确保配置正确**：
   ```cpp
   #ifndef TFT_MOSI
   #error "TFT_MOSI must be defined!"
   #endif
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
- SD 卡读取速度慢（使用高速卡，ESP32-S3 推荐使用 SDMMC 模式）
- 考虑增加 UI 任务优先级或栈大小

### 串口命令无响应
**可能原因：**
- 波特率不匹配（确认为 115200）
- 系统任务崩溃（查看 `task stats`）
- 串口被其他程序占用
- 看门狗复位（检查日志）
- ESP32-S3 USB CDC 模式：串口监视器断开后设备卡住（使用发布版固件）

**解决方法：**
```bash
# 检查任务状态
task stats

# 查看日志
log lines 50

# ESP32-S3: 使用发布版固件（禁用 CDC）
pio run -e esp32-s3-devkitc-1 --target upload
```

### SD 卡读取失败
**可能原因：**
- SD 卡格式不是 FAT32
- SD 卡接触不良
- 文件路径错误
- SD 卡容量过大（建议 ≤32GB）
- ESP32-S3: SDMMC 引脚连接错误

**解决方法：**
```bash
# 查看 SD 卡目录
tree / 2

# 查看日志中的错误
log cat

# 检查文件是否存在
file info /birds/1001/0.bin

# ESP32-S3: 检查 SDMMC 引脚映射
# CLK=36, CMD=35, D0=37（见 hardware_config.h）
```

### 小鸟动画不显示
**可能原因：**
- 图片文件缺失或损坏
- 配置文件错误
- 内存不足
- SD 卡未正确挂载

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

### IMU 传感器无数据
**症状：**RGB LED 显示黄灯，日志提示 "IMU returns zero data"

**可能原因：**
- 传感器未正确连接
- I2C 地址冲突
- 传感器型号不支持

**解决方法：**
```bash
# 查看日志，确认检测到的传感器型号
log lines 50

# 检查硬件连接
# ESP32: SDA=32, SCL=33
# ESP32-S3: SDA=1, SCL=2

# 手动测试传感器（查看启动日志中的 IMU Test 信息）
```

### ESP32-S3 SDMMC 初始化失败
**症状：**日志显示 "SDMMC mount failed, trying SPI mode..."

**解决方法：**
1. 检查 SDMMC 引脚连接（CLK=36, CMD=35, D0=37）
2. 确认 SD 卡兼容性（部分卡不支持 SDMMC）
3. 系统会自动回退到 SPI 模式，仍可正常工作

## 📝 更新日志

查看详细更新历史：[CHANGELOG.md](CHANGELOG.md)

### 最新版本 v1.1.0 (2025-12-16)

**✨ ESP32-S3 平台支持**
- 新增 ESP32-S3 DevKitC-1 编译环境
- SD 卡 SDMMC 高速模式支持（最高 40MHz）
- 新增 QMI8658 传感器支持
- 硬件抽象层 (HAL) 实现跨平台适配

**🔧 优化改进**
- 自动硬件检测和适配
- 多模式回退机制 (SDMMC → SPI)
- RGB LED 启动状态指示增强
- 显示驱动切换至 LovyanGFX

**📚 文档更新**
- 更新硬件支持说明
- 新增 ESP32-S3 引脚映射表
- 完善故障排查指南

### 版本 v1.0.0 (2025-12-09)
- 🎉 首个正式版本发布
- 双核 FreeRTOS 架构
- 完整的观鸟系统实现
- Python CLI 工具发布

## 📚 相关文档

### 核心文档
- [双核架构详细说明](docs/DUAL_CORE_ARCHITECTURE.md) - FreeRTOS 双核设计
- [开发指南](CLAUDE.md) - AI 辅助开发说明
- [更新日志](CHANGELOG.md) - 版本更新历史

### 功能文档
- [观鸟系统流程图](docs/BIRD_WATCHING_FLOW_DIAGRAM.md) - 系统流程详解
- [测试指南](docs/bird_watching_test_guide.md) - 功能测试步骤
- [统计视图指南](docs/STATS_VIEW_GUIDE.md) - 统计界面使用
- [添加小鸟指引](docs/GUIDE_TO_ADD_NEW_BIRDS.md) - 自定义小鸟资源

### 工具文档
- [CLI 工具使用](scripts/README_CLI_TOOLS.md) - 命令行工具指南
- [文件传输指南](docs/FILE_TRANSFER_GUIDE.md) - 文件上传下载
- [字体修改指南](docs/CHANGE_FONT_SIZE.md) - 自定义字体大小

### 技术分析
- [LVGL 9.x 升级分析](docs/LVGL_9X_UPGRADE_ANALYSIS.md) - 版本升级注意事项

### API 参考
主要类和接口：
- `TaskManager` - 双核任务管理器
- `HALManager` - 硬件抽象层管理器
- `BirdManager` - 观鸟系统管理器
- `LogManager` - 日志系统管理器
- `SerialCommands` - 串口命令处理器
- `IMUInterface` - IMU 传感器接口
- `SDInterface` - SD 卡接口

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
