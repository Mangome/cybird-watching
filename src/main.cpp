/**
 * @file main.cpp
 * @brief Cybird Watching 主程序 - 完整功能版本
 * 
 * 逐步启用外设：
 * 1. 基础系统 (Serial/日志)
 * 2. HAL 硬件抽象层 (IMU/SD检测)
 * 3. 显示屏 (TFT)
 * 4. SD卡
 * 5. RGB LED
 * 6. 环境光传感器
 */

#include <Arduino.h>
#include "config/hardware_config.h"
#include "system/logging/log_manager.h"
#include "system/tasks/task_manager.h"
#include "system/commands/serial_commands.h"
#include "hal/hal_manager.h"
#include "hal/sd_interface.h"
#include "drivers/display/display.h"
#include "drivers/sensors/imu/imu.h"
#include "drivers/io/rgb_led/rgb_led.h"
#include "applications/modules/bird_watching/core/bird_watching.h"
#include "applications/gui/core/gui_guider.h"  // 定义 lv_ui 类型

#ifdef __cplusplus
extern "C" {
#endif
    void lv_init_gui(void);          // GUI初始化（显示logo和小鸟）
    void lv_check_logo_timeout(void); // logo超时检查
#ifdef __cplusplus
}
#endif

// ==================== 功能开关 ====================
#define ENABLE_HAL              1   // 硬件抽象层 (IMU/SD检测)
#define ENABLE_DISPLAY          1   // TFT显示屏
#define ENABLE_SD_CARD          1   // SD卡（GUI资源需要）
#define ENABLE_RGB_LED          1   // RGB LED
#define ENABLE_AMBIENT_SENSOR   0   // 环境光传感器（暂时禁用）

// ==================== 全局对象 ====================
HAL::HALManager& hal = HAL::HALManager::getInstance();

// Display对象（TaskManager需要名为screen的全局对象）
Display screen;

// IMU对象（TaskManager需要）
IMU mpu;

// RGB LED对象（BirdManager需要）
Pixel rgb;

// ==================== 系统初始化 ====================
void setupSerial() {
    Serial.begin(115200);
    delay(1000);  // 等待USB CDC稳定
    
    Serial.println("\n\n╔════════════════════════════════════════╗");
    Serial.println("║   Cybird Watching System Boot         ║");
    Serial.println("╠════════════════════════════════════════╣");
    Serial.print("║ Platform:    ");
    Serial.print(HardwareConfig::getPlatformName());
    for(int i = strlen(HardwareConfig::getPlatformName()); i < 25; i++) Serial.print(" ");
    Serial.println("║");
    
    Serial.print("║ Chip Model:  ");
    Serial.print(ESP.getChipModel());
    Serial.print(" Rev ");
    Serial.print(ESP.getChipRevision());
    for(int i = strlen(ESP.getChipModel()) + 6; i < 25; i++) Serial.print(" ");
    Serial.println("║");
    
    Serial.print("║ CPU Freq:    ");
    Serial.print(ESP.getCpuFreqMHz());
    Serial.print(" MHz");
    for(int i = String(ESP.getCpuFreqMHz()).length() + 4; i < 25; i++) Serial.print(" ");
    Serial.println("║");
    
    Serial.print("║ Flash:       ");
    Serial.print(ESP.getFlashChipSize() / (1024 * 1024));
    Serial.print(" MB");
    for(int i = String(ESP.getFlashChipSize() / (1024 * 1024)).length() + 3; i < 25; i++) Serial.print(" ");
    Serial.println("║");
    
    Serial.print("║ PSRAM:       ");
    Serial.print(ESP.getPsramSize() / (1024 * 1024));
    Serial.print(" MB");
    for(int i = String(ESP.getPsramSize() / (1024 * 1024)).length() + 3; i < 25; i++) Serial.print(" ");
    Serial.println("║");
    
    Serial.print("║ Free Heap:   ");
    Serial.print(ESP.getFreeHeap() / 1024);
    Serial.print(" KB");
    for(int i = String(ESP.getFreeHeap() / 1024).length() + 3; i < 25; i++) Serial.print(" ");
    Serial.println("║");
    
    Serial.println("╚════════════════════════════════════════╝\n");
}

void setupLogging() {
    LogManager::getInstance()->setLogLevel(LogManager::LM_LOG_DEBUG);
    LOG_INFO("MAIN", "Log system initialized");
}

// ==================== Arduino Setup ====================
void setup() {
    // 1. 串口和日志系统
    setupSerial();
    setupLogging();
    
    LOG_INFO("MAIN", "========================================");
    LOG_INFO("MAIN", "Starting peripheral initialization...");
    LOG_INFO("MAIN", "========================================");
    
    // 2. RGB LED（提前初始化用于调试指示）
#if ENABLE_RGB_LED
    LOG_INFO("MAIN", "Initializing RGB LED...");
    rgb.init();
    
    // 启动指示：蓝灯闪 1 次（300ms 更明显）
    rgb.flashBlue(300);
    delay(300);
#endif
    
    // 3. HAL 硬件抽象层
#if ENABLE_HAL
    LOG_INFO("MAIN", "Initializing HAL...");
    
    if (!hal.initialize()) {
        LOG_ERROR("MAIN", "HAL initialization failed!");
#if ENABLE_RGB_LED
        // HAL 初始化失败，闪烁红灯3次
        for (int i = 0; i < 3; i++) {
            rgb.flashRed(200);
            delay(200);
        }
#endif
    } else {
        LOG_INFO("MAIN", "HAL initialization successful");
        
        // 测试IMU读取
        HAL::IMUInterface* imu = hal.getIMU();
        if (imu != nullptr) {
            LOG_INFO("MAIN", "IMU instance found, testing read...");
            
            // 等待传感器稳定并读取多次
            delay(50);
            for (int i = 0; i < 5; i++) {
                imu->update(10);
                delay(10);
            }
            
            int16_t ax = imu->getAccelX();
            int16_t ay = imu->getAccelY();
            int16_t az = imu->getAccelZ();
            int16_t gx = imu->getGyroX();
            int16_t gy = imu->getGyroY();
            int16_t gz = imu->getGyroZ();
            
            LOG_INFO("MAIN", String("IMU Test - Accel: X=") + String(ax) + 
                     ", Y=" + String(ay) + ", Z=" + String(az));
            LOG_INFO("MAIN", String("IMU Test - Gyro:  X=") + String(gx) + 
                     ", Y=" + String(gy) + ", Z=" + String(gz));
            
            // 检查数据是否有效
            if (ax != 0 || ay != 0 || az != 0) {
                LOG_INFO("MAIN", "IMU data valid!");
#if ENABLE_RGB_LED
                // IMU 数据有效，快速闪烁绿灯2次
                rgb.flashGreen(50);
                delay(100);
                rgb.flashGreen(50);
#endif
            } else {
                LOG_WARN("MAIN", "IMU returns zero data - sensor may need warm-up");
#if ENABLE_RGB_LED
                // 数据异常，闪烁黄灯（红+绿）
                rgb.flash(255, 255, 0, 100);
                delay(100);
                rgb.flash(255, 255, 0, 100);
#endif
            }
        } else {
            LOG_ERROR("MAIN", "IMU initialization failed - no sensor detected!");
#if ENABLE_RGB_LED
            // IMU 检测失败，闪烁红灯2次
            for (int i = 0; i < 2; i++) {
                rgb.flashRed(150);
                delay(150);
            }
#endif
        }
    }
#endif

    // 3.5. 初始化驱动层 IMU（TaskManager 需要）
    LOG_INFO("MAIN", "Initializing IMU driver...");
    mpu.init();
    if (!IMU::isInitialized()) {
        LOG_ERROR("MAIN", "IMU driver initialization failed!");
#if ENABLE_RGB_LED
        rgb.flashRed(300);
#endif
    } else {
        LOG_INFO("MAIN", "IMU driver initialized successfully");
    }

    // 4. SD卡（必须在显示屏之前初始化，避免SPI冲突）
#if ENABLE_SD_CARD
    LOG_INFO("MAIN", "Initializing SD card...");
    delay(500);  // 关键：增加延迟确保供电稳定
    if (!HAL::SDInterface::init()) {
        LOG_ERROR("MAIN", "SD card initialization failed!");
    } else {
        LOG_INFO("MAIN", String("SD card mounted: ") + HAL::SDInterface::getModeName());
        
        // 关键：通知LogManager SD卡已可用
        LOG_INFO("MAIN", "Re-initializing log manager with SD card support...");
        LogManager::getInstance()->setLogOutput(LogManager::OUTPUT_SD_CARD);
    }
#endif

    // 5. 显示屏和GUI
#if ENABLE_DISPLAY
    LOG_INFO("MAIN", "Initializing display...");
    screen.init();
    screen.setBackLight(0.2);  // 与backup版本一致：20%亮度
    LOG_INFO("MAIN", "Display initialized successfully");
    
    // 初始化GUI（显示logo和小鸟界面）
    LOG_INFO("MAIN", "Initializing GUI...");
    lv_init_gui();
    LOG_INFO("MAIN", "GUI initialized successfully");
    
    // 初始化串口命令系统（必须在TaskManager之前初始化）
    LOG_INFO("MAIN", "Initializing Serial Commands...");
    SerialCommands::getInstance()->initialize();
    
    // 初始化TaskManager（BirdWatching需要LVGL互斥锁）
    LOG_INFO("MAIN", "Initializing Task Manager...");
    TaskManager* taskMgr = TaskManager::getInstance();
    if (!taskMgr->initialize()) {
        LOG_ERROR("MAIN", "Task Manager initialization failed!");
    } else {
        LOG_INFO("MAIN", "Task Manager initialized successfully");
        
        // 启动UI和System任务
        if (!taskMgr->startTasks()) {
            LOG_ERROR("MAIN", "Failed to start tasks!");
        } else {
            LOG_INFO("MAIN", "Tasks started successfully");
        }
    }
    
    // 初始化小鸟系统（会扫描资源并自动隐藏logo）
    LOG_INFO("MAIN", "Initializing Bird Watching System...");
    if (BirdWatching::initializeBirdWatching(guider_ui.scenes_canvas)) {
        LOG_INFO("MAIN", "Bird Watching System initialized successfully");
    } else {
        LOG_ERROR("MAIN", "Bird Watching System initialization failed!");
    }
#endif

    // 6. 运行RGB LED测试序列
#if ENABLE_RGB_LED
    LOG_INFO("MAIN", "Running RGB LED test sequence...");
    rgb.testSequence();
#endif

    // 7. 环境光传感器
#if ENABLE_AMBIENT_SENSOR
    LOG_INFO("MAIN", "Initializing ambient light sensor...");
    // TODO: 添加环境光传感器初始化
#endif

    LOG_INFO("MAIN", "========================================");
    LOG_INFO("MAIN", "System initialization complete!");
    LOG_INFO("MAIN", "========================================\n");
}
void loop() {
    // 在双核架构下，主loop可以空闲或处理其他低优先级任务
    // 所有核心功能已经在FreeRTOS任务中运行：
    // - Core 0: UI Task (LVGL + Display)
    // - Core 1: System Task (Sensors + Commands + Bird Watching)
    
    // 注意：串口命令处理已由SystemTask接管，不要在loop()中重复调用！
    // 否则会导致竞态条件和命令重复执行
    
    // 可选：定期打印任务统计信息
    static unsigned long lastStatsTime = 0;
    unsigned long currentTime = millis();
    
    if (currentTime - lastStatsTime >= 60000) { // 每60秒打印一次
        TaskManager* taskManager = TaskManager::getInstance();
        if (taskManager) {
            taskManager->printTaskStats();
        }
        lastStatsTime = currentTime;
    }
    
    // 让出CPU时间给FreeRTOS任务
    delay(100);
}
