#include <Arduino.h>
#include "esp_task_wdt.h"
#include "drivers/display/display.h"
#include "drivers/sensors/imu/imu.h"
#include "drivers/sensors/ambient/ambient.h"
#include "drivers/communication/network/network.h"
#include "drivers/storage/sd_card/sd_card.h"
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
SdCard tf;
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
    Serial.println("*** FIRMWARE v3.0 - DUAL CORE ARCHITECTURE ***");
    Serial.println("Core 0: UI Rendering | Core 1: System Logic");
    delay(1000);

    // 初始化日志系统 - 先只用串口输出，等SD卡初始化完成后再启用SD卡
    LogManager* logManager = LogManager::getInstance();
    logManager->initialize(LogManager::LM_LOG_INFO, LogManager::OUTPUT_SERIAL);

    LOG_INFO("MAIN", "=== CybirdWatching Starting ===");
    LOG_INFO("MAIN", "*** FIRMWARE v3.0 - DUAL CORE ARCHITECTURE ***");
    delay(1000);
    LOG_INFO("MAIN", "Serial communication OK");

    // 初始化串口命令系统
    SerialCommands* serialCommands = SerialCommands::getInstance();
    serialCommands->initialize();

    /*** Init screen ***/
    LOG_INFO("MAIN", "Initializing screen...");
    screen.init();
    LOG_INFO("MAIN", "Screen initialized");

    LOG_INFO("MAIN", "Setting backlight...");
    screen.setBackLight(0.2);
    LOG_INFO("MAIN", "Backlight set");

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

    /*** Init micro SD-Card ***/
    LOG_INFO("MAIN", "Initializing SD card...");
    tf.init();
    LOG_INFO("MAIN", "SD card initialized");

    // 通知LogManager SD卡已初始化
    LOG_INFO("MAIN", "Re-initializing log manager with SD card support...");
    // Use SD card only to keep CLI responses clean
    // Use 'log cat' command to view full log when needed
    logManager->setLogOutput(LogManager::OUTPUT_SD_CARD);

    LOG_INFO("MAIN", "Initializing LVGL file system...");
    lv_fs_if_init();
    LOG_INFO("MAIN", "LVGL file system initialized");

    // LOG_INFO("MAIN", "Reading WiFi configuration...");
    // String ssid = tf.readFileLine("/wifi.txt", 1);        // line-1 for WiFi ssid
    // String password = tf.readFileLine("/wifi.txt", 2);    // line-2 for WiFi password
    // LOG_INFO("MAIN", "WiFi configuration read");

    /*** Inflate GUI objects ***/
    LOG_INFO("MAIN", "Creating GUI...");
    lv_init_gui();
    LOG_INFO("MAIN", "GUI created");
    setup_ui(&guider_ui);

    /*** Init Task Manager FIRST (creates LVGL mutex) ***/
    LOG_INFO("MAIN", "Initializing Task Manager...");
    taskManager = TaskManager::getInstance();
    
    if (!taskManager->initialize()) {
        LOG_ERROR("MAIN", "Failed to initialize Task Manager");
        return;
    }
    LOG_INFO("MAIN", "Task Manager initialized (LVGL mutex created)");

    /*** Init Bird Watching System (requires LVGL mutex) ***/
    LOG_INFO("MAIN", "Initializing Bird Watching System...");
    // 传入scenes给BirdManager作为显示对象（统计界面的父对象）
    if (BirdWatching::initializeBirdWatching(guider_ui.scenes)) {
        LOG_INFO("MAIN", "Bird Watching System initialized successfully");
    } else {
        LOG_ERROR("MAIN", "Failed to initialize Bird Watching System");
    }

    /*** Start Dual-Core Tasks ***/
    LOG_INFO("MAIN", "Starting dual-core tasks...");

    if (!taskManager->startTasks()) {
        LOG_ERROR("MAIN", "Failed to start tasks");
        return;
    }
    LOG_INFO("MAIN", "Dual-core tasks started successfully");
    LOG_INFO("MAIN", "  - Core 0: UI Task (LVGL + Display + Animation)");
    LOG_INFO("MAIN", "  - Core 1: System Task (Sensors + Commands + Business Logic)");

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