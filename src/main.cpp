#include <Arduino.h>
#include "esp_task_wdt.h"
#include "config/version.h"
#include "drivers/display/display.h"
#include "drivers/sensors/imu/imu.h"
#include "drivers/sensors/ambient/ambient.h"
#include "drivers/communication/network/network.h"
#include "hal/sd_interface.h"
#include "drivers/io/rgb_led/rgb_led.h"
#include "system/lvgl/ports/lv_port_indev.h"
#include "system/lvgl/ports/lv_port_fatfs.h"
#include "applications/gui/core/lv_cubic_gui.h"
#include "applications/gui/core/gui_guider.h"
#include "system/logging/log_manager.h"
#include "system/commands/serial_commands.h"
#include "applications/modules/bird_watching/core/bird_watching.h"
#include "system/tasks/task_manager.h"

/*** Component objects ***/
Display screen;
IMU mpu;
Pixel rgb;
Network wifi;

lv_ui guider_ui;

/*** Task Manager ***/
TaskManager* taskManager = nullptr;

void setup()
{
    // 配置看门狗超时时间为10秒（避免图像加载时触发看门狗）
    esp_task_wdt_init(10, true);
    // 初始化串口通信
    Serial.begin(115200);
    delay(1000); // 等待串口稳定

    // 立即输出标识，不依赖日志系统
    Serial.println("=== CybirdWatching Starting ===");
    Serial.println(FIRMWARE_BANNER);
    Serial.println("Core 0: UI Rendering | Core 1: System Logic");
    delay(1000);

    // 初始化日志系统 - 先只用串口输出，等SD卡初始化完成后再启用SD卡
    LogManager* logManager = LogManager::getInstance();
    logManager->initialize(LogManager::LM_LOG_INFO, LogManager::OUTPUT_SERIAL);

    LOG_INFO("MAIN", "=== CybirdWatching Starting ===");
    LOG_INFO("MAIN", FIRMWARE_BANNER);
    delay(1000);
    LOG_INFO("MAIN", "Serial communication OK");

    // 初始化串口命令系统
    SerialCommands* serialCommands = SerialCommands::getInstance();
    serialCommands->initialize();

    /*** Init micro SD-Card EARLY (before screen to avoid SPI conflicts) ***/
    LOG_INFO("MAIN", "Initializing SD card...");
    // 在烧录后添加额外延迟，让硬件稳定
    delay(500);  // 增加延迟确保供电稳定
    
    if (!HAL::SDInterface::init()) {
        LOG_ERROR("MAIN", "SD card initialization failed!");
    } else {
        LOG_INFO("MAIN", "SD card mounted in " + String(HAL::SDInterface::getModeName()) + " mode");
    }

    // 通知LogManager SD卡已初始化
    LOG_INFO("MAIN", "Re-initializing log manager with SD card support...");
    // Use SD card only to keep CLI responses clean
    // Use 'log cat' command to view full log when needed
    logManager->setLogOutput(LogManager::OUTPUT_SD_CARD);

    /*** Init screen ***/
    LOG_INFO("MAIN", "Initializing screen...");
    screen.init();
    LOG_INFO("MAIN", "Screen initialized");

    LOG_INFO("MAIN", "Setting backlight...");
    screen.setBackLight(0.2);
    LOG_INFO("MAIN", "Backlight set");

    /*** Init LVGL file system ***/
    LOG_INFO("MAIN", "Initializing LVGL file system...");
    lv_fs_if_init();
    LOG_INFO("MAIN", "LVGL file system initialized");

    /*** Init IMU as input device ***/
    LOG_INFO("MAIN", "Initializing LVGL input device...");
    lv_port_indev_init();
    LOG_INFO("MAIN", "LVGL input device initialized");

    LOG_INFO("MAIN", "Initializing MPU...");
    mpu.init();
    LOG_INFO("MAIN", "MPU initialized");

    /*** Init on-board RGB ***/
    LOG_INFO("MAIN", "Initializing RGB LED...");
    rgb.init();
    LOG_INFO("MAIN", "RGB LED initialized (default: OFF)");

    // LOG_INFO("MAIN", "Reading WiFi configuration...");
    // String ssid = HAL::SDInterface::readFileLine("/wifi.txt", 1);        // line-1 for WiFi ssid
    // String password = HAL::SDInterface::readFileLine("/wifi.txt", 2);    // line-2 for WiFi password
    // LOG_INFO("MAIN", "WiFi configuration read");

    /*** Inflate GUI objects ***/
    LOG_INFO("MAIN", "Creating GUI...");
    setup_ui(&guider_ui);  // 创建UI界面(包括scenes)
    LOG_INFO("MAIN", "GUI UI created");

    /*** Init Task Manager FIRST (creates LVGL mutex) ***/
    LOG_INFO("MAIN", "Initializing Task Manager...");
    taskManager = TaskManager::getInstance();
    
    if (!taskManager->initialize()) {
        LOG_ERROR("MAIN", "Failed to initialize Task Manager");
        return;
    }
    LOG_INFO("MAIN", "Task Manager initialized (LVGL mutex created)");

    /*** Start Dual-Core Tasks EARLY ***/
    LOG_INFO("MAIN", "Starting dual-core tasks...");

    if (!taskManager->startTasks()) {
        LOG_ERROR("MAIN", "Failed to start tasks");
        return;
    }
    LOG_INFO("MAIN", "Dual-core tasks started successfully");
    LOG_INFO("MAIN", "  - Core 0: UI Task (LVGL + Display + Animation)");
    LOG_INFO("MAIN", "  - Core 1: System Task (Sensors + Commands + Business Logic)");

    // ⚠️ 重要：先加载并显示logo（在扫描资源之前）
    LOG_INFO("MAIN", "Loading and displaying logo...");
    lv_init_gui();  // 尝试加载logo(如果SD卡可用),否则显示小鸟界面
    LOG_INFO("MAIN", "Logo displayed, starting to scan bird resources...");

    /*** Init Bird Watching System (扫描小鸟资源期间logo持续显示) ***/
    LOG_INFO("MAIN", "Initializing Bird Watching System (scanning bird resources)...");
    // 传入scenes给BirdManager作为显示对象（统计界面的父对象）
    if (BirdWatching::initializeBirdWatching(guider_ui.scenes)) {
        LOG_INFO("MAIN", "Bird Watching System initialized successfully");
    } else {
        LOG_ERROR("MAIN", "Failed to initialize Bird Watching System");
    }
    LOG_INFO("MAIN", "Bird resources scan completed");
    
    // 扫描完成后立即关闭logo，显示小鸟界面
    LOG_INFO("MAIN", "Closing logo after resource scan...");
    lv_hide_logo();
    LOG_INFO("MAIN", "Logo closed, bird interface ready");

    LOG_INFO("MAIN", "Setup completed, tasks running...");
    
    // 打印任务统计信息
    delay(2000);
    taskManager->printTaskStats();
}

void loop()
{
    // 在双核架构下，主loop可以空闲或处理其他低优先级任务
    // 所有核心功能已经在FreeRTOS任务中运行：
    // - Core 0: UI Task (200Hz - LVGL + Display)
    // - Core 1: System Task (100Hz - Sensors + Commands + Business Logic)
    
    // 可选：定期打印任务统计信息
    static unsigned long lastStatsTime = 0;
    unsigned long currentTime = millis();
    
    if (currentTime - lastStatsTime >= 60000) { // 每60秒打印一次
        if (taskManager) {
            taskManager->printTaskStats();
        }
        lastStatsTime = currentTime;
    }
    
    // 让出CPU给FreeRTOS调度器
    delay(1000);
}