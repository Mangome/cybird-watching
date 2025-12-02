#include "task_manager.h"
#include "system/logging/log_manager.h"
#include "drivers/display/display.h"
#include "drivers/sensors/imu/imu.h"
#include "drivers/io/rgb_led/rgb_led.h"
#include "system/commands/serial_commands.h"
#include "applications/modules/bird_watching/core/bird_watching.h"

// 外部对象引用(在main.cpp中定义)
extern Display screen;
extern IMU mpu;
extern Pixel rgb;

TaskManager* TaskManager::instance_ = nullptr;

TaskManager::TaskManager()
    : ui_task_handle_(nullptr)
    , system_task_handle_(nullptr)
    , ui_queue_(nullptr)
    , system_queue_(nullptr)
    , lvgl_mutex_(nullptr)
{
}

TaskManager::~TaskManager()
{
    if (ui_queue_) {
        vQueueDelete(ui_queue_);
    }
    if (system_queue_) {
        vQueueDelete(system_queue_);
    }
    if (lvgl_mutex_) {
        vSemaphoreDelete(lvgl_mutex_);
    }
}

TaskManager* TaskManager::getInstance()
{
    if (!instance_) {
        instance_ = new TaskManager();
    }
    return instance_;
}

bool TaskManager::initialize()
{
    LOG_INFO("TASK_MGR", "Initializing Task Manager...");

    // 创建消息队列
    ui_queue_ = xQueueCreate(10, sizeof(TaskMessage));
    if (!ui_queue_) {
        LOG_ERROR("TASK_MGR", "Failed to create UI queue");
        return false;
    }

    system_queue_ = xQueueCreate(20, sizeof(TaskMessage));
    if (!system_queue_) {
        LOG_ERROR("TASK_MGR", "Failed to create System queue");
        return false;
    }

    // 创建LVGL互斥锁
    lvgl_mutex_ = xSemaphoreCreateMutex();
    if (!lvgl_mutex_) {
        LOG_ERROR("TASK_MGR", "Failed to create LVGL mutex");
        return false;
    }

    LOG_INFO("TASK_MGR", "Task Manager initialized successfully");
    return true;
}

bool TaskManager::startTasks()
{
    LOG_INFO("TASK_MGR", "Starting tasks...");

    // 创建UI任务 (Core 0 - Protocol Core)
    BaseType_t result = xTaskCreatePinnedToCore(
        uiTaskFunction,           // 任务函数
        "UI_Task",                // 任务名称
        UI_TASK_STACK_SIZE,       // 栈大小
        this,                     // 参数
        UI_TASK_PRIORITY,         // 优先级
        &ui_task_handle_,         // 任务句柄
        UI_TASK_CORE              // 绑定到Core 0
    );

    if (result != pdPASS) {
        LOG_ERROR("TASK_MGR", "Failed to create UI task");
        return false;
    }
    LOG_INFO("TASK_MGR", "UI Task created on Core 0");

    // 创建系统任务 (Core 1 - Application Core)
    result = xTaskCreatePinnedToCore(
        systemTaskFunction,       // 任务函数
        "System_Task",            // 任务名称
        SYSTEM_TASK_STACK_SIZE,   // 栈大小
        this,                     // 参数
        SYSTEM_TASK_PRIORITY,     // 优先级
        &system_task_handle_,     // 任务句柄
        SYSTEM_TASK_CORE          // 绑定到Core 1
    );

    if (result != pdPASS) {
        LOG_ERROR("TASK_MGR", "Failed to create System task");
        return false;
    }
    LOG_INFO("TASK_MGR", "System Task created on Core 1");

    LOG_INFO("TASK_MGR", "All tasks started successfully");
    return true;
}

bool TaskManager::sendToUITask(const TaskMessage& msg)
{
    if (!ui_queue_) {
        return false;
    }
    return xQueueSend(ui_queue_, &msg, pdMS_TO_TICKS(100)) == pdTRUE;
}

bool TaskManager::sendToSystemTask(const TaskMessage& msg)
{
    if (!system_queue_) {
        return false;
    }
    return xQueueSend(system_queue_, &msg, pdMS_TO_TICKS(100)) == pdTRUE;
}

bool TaskManager::takeLVGLMutex(uint32_t timeout_ms)
{
    if (!lvgl_mutex_) {
        return false;
    }
    return xSemaphoreTake(lvgl_mutex_, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

void TaskManager::giveLVGLMutex()
{
    if (lvgl_mutex_) {
        xSemaphoreGive(lvgl_mutex_);
    }
}

void TaskManager::printTaskStats()
{
    char buffer[256];
    
    LOG_INFO("TASK_MGR", "=== Task Statistics ===");
    
    if (ui_task_handle_) {
        UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(ui_task_handle_);
        snprintf(buffer, sizeof(buffer), "UI Task - Stack free: %u bytes", stackHighWaterMark);
        LOG_INFO("TASK_MGR", buffer);
    }
    
    if (system_task_handle_) {
        UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(system_task_handle_);
        snprintf(buffer, sizeof(buffer), "System Task - Stack free: %u bytes", stackHighWaterMark);
        LOG_INFO("TASK_MGR", buffer);
    }
    
    snprintf(buffer, sizeof(buffer), "Free heap: %u bytes", ESP.getFreeHeap());
    LOG_INFO("TASK_MGR", buffer);
}

/**
 * @brief UI任务函数 - 运行在Core 0
 * 
 * 职责:
 * - LVGL GUI更新 (lv_timer_handler)
 * - Display驱动 (screen.routine)
 * - BirdAnimation动画播放
 * - 图片解码和渲染
 */
void TaskManager::uiTaskFunction(void* parameter)
{
    TaskManager* manager = static_cast<TaskManager*>(parameter);
    LOG_INFO("UI_TASK", "UI Task started on Core 0");

    TaskMessage msg;
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t taskPeriod = pdMS_TO_TICKS(5); // 5ms周期 = 200Hz

    while (true) {
        // 处理消息队列(非阻塞)
        while (xQueueReceive(manager->ui_queue_, &msg, 0) == pdTRUE) {
            // 处理UI相关消息
            switch (msg.type) {
                case MSG_TRIGGER_BIRD:
                    // 触发小鸟动画
                    if (manager->takeLVGLMutex(100)) {
                        BirdWatching::triggerBird();
                        manager->giveLVGLMutex();
                    }
                    break;
                
                default:
                    break;
            }
        }

        // 获取LVGL互斥锁并更新UI
        if (manager->takeLVGLMutex(10)) {
            // 处理BirdWatching触发请求(必须在UI任务中执行)
            // 注意: 图像加载可能耗时较长，但已优化为分块加载
            BirdWatching::processBirdTriggerRequest();

            // LVGL定时器处理
            lv_timer_handler();
            
            // Display驱动更新
            screen.routine();
            
            manager->giveLVGLMutex();
        } else {
            // 如果无法获取互斥锁，让出CPU避免死锁
            vTaskDelay(1);
        }

        // 精确延时,保证稳定的刷新率
        vTaskDelayUntil(&lastWakeTime, taskPeriod);
    }
}

/**
 * @brief 系统任务函数 - 运行在Core 1
 * 
 * 职责:
 * - IMU传感器更新
 * - 串口命令处理
 * - WiFi网络通信
 * - SD卡操作
 * - BirdManager业务逻辑
 * - 统计数据管理
 */
void TaskManager::systemTaskFunction(void* parameter)
{
    TaskManager* manager = static_cast<TaskManager*>(parameter);
    LOG_INFO("SYS_TASK", "System Task started on Core 1");

    TaskMessage msg;
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t taskPeriod = pdMS_TO_TICKS(10); // 10ms周期 = 100Hz

    unsigned long lastMPUUpdate = 0;
    const unsigned long MPU_UPDATE_INTERVAL = 200; // 200ms更新一次IMU

    while (true) {
        unsigned long currentTime = millis();

        // 处理消息队列(非阻塞)
        while (xQueueReceive(manager->system_queue_, &msg, 0) == pdTRUE) {
            // 处理系统相关消息
            switch (msg.type) {
                case MSG_SHOW_STATS:
                    BirdWatching::showBirdStatistics();
                    break;
                
                case MSG_GESTURE_EVENT:
                    // 处理手势事件
                    break;
                
                default:
                    break;
            }
        }

        // 更新IMU数据 (200ms间隔)
        if (currentTime - lastMPUUpdate >= MPU_UPDATE_INTERVAL) {
            mpu.update(0); // 不使用内部延时
            lastMPUUpdate = currentTime;

            // 检测手势并触发相应事件
            GestureType gesture = mpu.detectGesture();
            if (gesture == GESTURE_LEFT_RIGHT_TILT) {
                // 左右倾斜 - 触发小鸟（10秒CD）
                LOG_INFO("SYS_TASK", "Left/Right tilt detected, triggering bird");
                BirdWatching::triggerBird();
            }
        }

        // 更新Bird Watching系统
        BirdWatching::updateBirdWatching();

        // 处理串口命令
        SerialCommands::getInstance()->handleInput();

        // 任务延时
        vTaskDelayUntil(&lastWakeTime, taskPeriod);
    }
}
