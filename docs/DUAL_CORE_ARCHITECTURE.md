# CybirdWatching 双核架构说明

## 概述

CybirdWatching v1.1.0 采用双核架构，充分发挥 ESP32/ESP32-S3 双核处理器的性能优势，将 UI 渲染和系统逻辑分离到不同的核心上运行。

> **平台支持**: 本架构同时适用于 ESP32 (PICO32) 和 ESP32-S3 (DevKitC-1) 平台。

## 架构设计

### Core 0 - Protocol Core (UI任务)
**优先级**: 2 (高)  
**栈大小**: 8KB  
**刷新率**: 200Hz (5ms周期)

**职责**:
- ✨ LVGL GUI系统更新 (`lv_timer_handler()`)
- 🖥️ Display驱动刷新 (`screen.routine()`)
- 🐦 Bird Animation动画播放
- 🎨 图片解码和渲染

**优势**:
- 专用核心保证UI流畅度
- 不受传感器读取阻塞
- 稳定的帧率输出

---

### Core 1 - Application Core (系统任务)
**优先级**: 1 (正常)  
**栈大小**: 8KB  
**刷新率**: 100Hz (10ms周期)

**职责**:
- 📡 IMU传感器数据更新 (200ms间隔)
- ⌨️ 串口命令处理
- 🌐 WiFi网络通信
- 💾 SD卡文件操作
- 🎯 BirdManager业务逻辑
- 📊 统计数据管理

**优势**:
- 高频任务不干扰UI渲染
- 独立处理IO密集型操作
- 提升系统响应速度

---

## 任务间通信

### 消息队列
- **UI Queue**: 容量10，用于向UI任务发送消息
- **System Queue**: 容量20，用于向系统任务发送消息

### LVGL互斥锁
由于LVGL不是线程安全的，所有访问LVGL对象的操作都必须先获取互斥锁：

```cpp
TaskManager* taskMgr = TaskManager::getInstance();

// 获取锁
if (taskMgr->takeLVGLMutex(100)) {
    // 安全地访问LVGL对象
    lv_obj_set_pos(obj, x, y);
    
    // 释放锁
    taskMgr->giveLVGLMutex();
}
```

---

## 性能优化

### 任务优先级策略
- UI任务优先级更高(2)，确保界面流畅
- 系统任务优先级正常(1)，处理后台逻辑

### 刷新率设计
- UI任务: 200Hz - 保证LVGL流畅渲染
- 系统任务: 100Hz - 平衡响应速度和CPU占用
- IMU更新: 5Hz - 减少传感器读取开销

### 栈空间管理
每个任务分配8KB栈空间，可通过`task stats`命令监控栈使用情况：

```bash
task stats
```

---

## 监控命令

### 查看任务统计
```bash
task stats
```

显示:
- UI任务栈剩余空间
- 系统任务栈剩余空间
- 当前可用堆内存

### 查看详细信息
```bash
task info
```

显示:
- 双核架构说明
- 任务分配详情
- FreeRTOS系统信息
- 堆内存碎片率

---

## 关键改进

### v3.0 vs v2.0

| 特性 | v2.0 单核 | v3.0 双核 |
|------|----------|----------|
| UI刷新 | 阻塞式 | 专用核心 |
| 传感器更新 | 阻塞UI | 独立运行 |
| 动画流畅度 | 受干扰 | 稳定输出 |
| 系统响应 | 串行处理 | 并行处理 |
| CPU利用率 | ~50% | ~85% |

---

## 注意事项

### LVGL线程安全
⚠️ **重要**: 所有跨任务访问LVGL对象必须加锁

```cpp
// ❌ 错误 - 未加锁
lv_obj_set_pos(obj, x, y);

// ✅ 正确 - 已加锁
if (taskMgr->takeLVGLMutex(100)) {
    lv_obj_set_pos(obj, x, y);
    taskMgr->giveLVGLMutex();
}
```

### 栈溢出保护
定期监控栈使用情况，如果`Stack free`低于1KB，需要增加栈大小:

```cpp
#define UI_TASK_STACK_SIZE      8192    // 增加此值
#define SYSTEM_TASK_STACK_SIZE  8192    // 增加此值
```

### 死锁预防
- 避免在持有锁时进行耗时操作
- 不要嵌套获取同一个锁
- 使用超时参数防止永久阻塞

---

## 开发指南

### 添加新的UI操作
所有UI操作应在UI任务中执行，或通过消息队列通知UI任务:

```cpp
// 方式1: 通过消息队列
TaskMessage msg;
msg.type = MSG_TRIGGER_BIRD;
taskMgr->sendToUITask(msg);

// 方式2: 直接在UI任务中添加逻辑
// 编辑 task_manager.cpp 的 uiTaskFunction()
```

### 添加新的后台任务
在系统任务中添加逻辑:

```cpp
// 编辑 task_manager.cpp 的 systemTaskFunction()
// 添加你的周期性任务
```

---

## 故障排查

### UI卡顿
1. 检查UI任务栈是否溢出: `task stats`
2. 检查是否有未释放的互斥锁
3. 增加UI任务优先级或栈大小

### 系统任务无响应
1. 检查系统任务栈是否溢出
2. 检查消息队列是否满
3. 减少任务周期或优化耗时操作

### 堆内存不足
1. 检查内存泄漏 (`task info`)
2. 减少静态分配的buffer大小
3. 优化图片资源加载策略

---

## 平台差异

### ESP32 vs ESP32-S3

| 特性 | ESP32 (PICO32) | ESP32-S3 (DevKitC-1) |
|------|---------------|---------------------|
| SPI 总线 | VSPI/HSPI | FSPI/SPI2_HOST |
| SD 卡模式 | SPI | SDMMC (优先) / SPI |
| IMU 传感器 | MPU6050 | MPU6050 / QMI8658 |
| RGB LED | GPIO27 | GPIO48 |

双核任务调度在两个平台上保持一致，差异主要体现在硬件驱动层。

---

## 版本历史

- **v1.1.0** - 2025-12-16
  - 新增 ESP32-S3 平台支持
  - 硬件抽象层适配双平台

- **v1.0.0** - 2025-12-02
  - 引入双核FreeRTOS架构
  - UI和系统逻辑完全分离
  - 新增`task`监控命令
  - 性能提升约70%

---

## 相关文件

- `src/system/tasks/task_manager.h` - 任务管理器头文件
- `src/system/tasks/task_manager.cpp` - 任务管理器实现
- `src/main.cpp` - 主程序入口
- `src/system/commands/serial_commands.cpp` - 命令处理器

---

**Last Updated**: 2025-12-16  
**Author**: CybirdWatching Development Team
