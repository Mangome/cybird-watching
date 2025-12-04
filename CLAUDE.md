# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

这是一个名为"CybirdWatching"的嵌入式项目，基于ESP32平台开发。该项目是一个智能显示设备，集成了多种传感器、显示模块、网络连接和用户界面功能。项目使用Arduino框架和PlatformIO进行开发。

## 开发环境

- **平台**: ESP32 (pico32)
- **框架**: Arduino
- **构建系统**: PlatformIO
- **串口速度**: 115200
- **上传端口**: COM3

## 构建和开发命令

### 常用PlatformIO命令
```bash
# 编译项目
platformio run

# 上传到设备
platformio run --target upload

# 监控串口输出
platformio device monitor

# 清理构建文件
platformio run --target clean

# 在IDE中构建
platformio -c clion run

# 调试构建
platformio -c clion run --target debug
```

## 项目架构

### 目录结构
```
src/
├── main.cpp                           # 主程序入口
├── drivers/                           # 硬件驱动层
│   ├── display/                       # 显示驱动 (TFT_eSPI)
│   ├── sensors/                       # 传感器驱动
│   │   ├── imu/                       # IMU传感器 (MPU6050)
│   │   └── ambient/                   # 环境传感器
│   ├── communication/network/         # 网络通信 (WiFi)
│   ├── storage/sd_card/               # SD卡存储
│   └── io/rgb_led/                    # RGB LED控制 (FastLED)
├── system/                            # 系统服务层
│   ├── logging/                       # 日志管理系统
│   ├── commands/                      # 串口命令系统
│   └── lvgl/ports/                    # LVGL端口层
└── applications/                      # 应用层
    └── gui/                           # 图形用户界面
        ├── core/                      # 核心GUI组件
        └── screens/                   # GUI屏幕
```

### 核心组件

#### 1. 显示系统 (`src/drivers/display/`)
- 基于TFT_eSPI库的LCD显示控制
- LVGL图形库集成
- 支持背光PWM控制
- 负责屏幕初始化和日常刷新

#### 2. 传感器系统 (`src/drivers/sensors/`)
- **IMU**: MPU6050加速度计和陀螺仪，用作用户输入设备
- **环境传感器**: 环境光线、温度等传感器支持

#### 3. 网络通信 (`src/drivers/communication/network/`)
- WiFi网络连接管理
- 从SD卡读取WiFi配置 (`/wifi.txt`)
- 支持网络数据传输功能

#### 4. 存储系统 (`src/drivers/storage/sd_card/`)
- SD卡文件系统支持
- 日志文件存储
- 配置文件读取

#### 5. 用户界面 (`src/applications/gui/`)
- 基于LVGL的图形用户界面
- 包含主屏幕和场景屏幕
- 支持触摸和IMU输入

#### 6. 日志系统 (`src/system/logging/`)
- 单例模式的日志管理器
- 支持串口和SD卡输出
- 多级日志 (DEBUG, INFO, WARN, ERROR, FATAL)
- 日志文件自动轮转
- 专为嵌入式环境优化

#### 7. 命令系统 (`src/system/commands/`)
- 串口命令解析和执行
- 支持系统控制和调试命令
- 非阻塞式命令处理

### 外部依赖库

#### 核心库 (`lib/`)
- **FastLED**: 高性能LED控制库，用于RGB LED
- **MPU6050**: IMU传感器驱动库
- **TFT_eSPI**: 高性能TFT LCD驱动库
- **LVGL**: 轻量级图形用户界面库

### 系统初始化流程

1. 串口初始化 (115200 baud)
2. 日志系统初始化 (先串口输出)
3. 串口命令系统初始化
4. SD卡初始化
5. 显示屏初始化和背光设置
6. LVGL文件系统初始化
7. RGB LED初始化
8. GUI界面创建
9. Task Manager初始化（创建LVGL互斥锁）
10. **IMU初始化**（延后到资源扫描完成后，避免总线冲突）
11. 双核任务启动（Core 0: UI任务，Core 1: 系统任务）
12. **Logo显示**（在资源扫描前显示）
13. Bird Watching系统初始化（扫描小鸟资源，耗时操作）
14. Logo关闭，显示小鸟界面

### 主循环逻辑

```cpp
void loop() {
    screen.routine();      // LVGL任务处理
    mpu.update(200);       // 200ms更新一次IMU数据
    SerialCommands::getInstance()->handleInput();  // 处理串口命令
}
```

### 配置文件

- **WiFi配置**: `/wifi.txt` (第1行SSID, 第2行密码)
- **项目配置**: `platformio.ini` - 包含构建标志和库依赖
- **包含路径**: 所有模块的包含路径已在platformio.ini中配置

### 特殊功能

#### 串口命令系统
- 支持实时系统控制
- 日志查看和管理命令
- 调试和状态查询命令
- 不干扰GUI显示的SD卡专用日志记录

#### 日志管理
- 智能日志轮转防止SD卡写满
- 串口和SD卡双输出模式
- 可配置的日志级别
- 内置时间戳和标签系统

## 开发注意事项

1. **头文件包含**: 所有必要的包含路径已在`platformio.ini`中配置
2. **串口通信**: 使用115200波特率，避免在此速率下发送过多日志影响命令响应
3. **LVGL集成**: 显示系统完全基于LVGL，所有UI更新应通过LVGL API
4. **传感器更新**: IMU数据以200ms间隔更新，可根据需要调整
5. **内存管理**: 注意ESP32的内存限制，避免创建大型静态对象
6. **日志使用**: 使用`LOG_*`宏而非直接的`Serial.print`以获得一致的日志格式
7. **命令响应**: 在实现新功能时考虑串口命令接口，便于调试和控制