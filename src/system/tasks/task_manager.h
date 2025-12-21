#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

// 任务配置
#define UI_TASK_STACK_SIZE      8192    // UI任务栈大小(8KB)
#define SYSTEM_TASK_STACK_SIZE  16384   // 系统任务栈大小(16KB) - 增加以支持日志命令
#define UI_TASK_PRIORITY        2       // UI任务优先级
#define SYSTEM_TASK_PRIORITY    1       // 系统任务优先级
#define UI_TASK_CORE            0       // UI任务运行在Core 0 (Protocol Core)
#define SYSTEM_TASK_CORE        1       // 系统任务运行在Core 1 (Application Core)

// 任务间消息类型
enum TaskMessageType {
    MSG_TRIGGER_BIRD = 0,      // 触发小鸟动画
    MSG_UPDATE_CONFIG,         // 更新配置
    MSG_SHOW_STATS,           // 显示统计信息
    MSG_GESTURE_EVENT,        // 手势事件
    MSG_SYSTEM_EVENT          // 系统事件
};

// 任务间消息结构
struct TaskMessage {
    TaskMessageType type;
    uint32_t param1;
    uint32_t param2;
    void* data;
};

/**
 * @brief 双核任务管理器
 * 
 * 架构说明:
 * - Core 0: UI渲染任务 (LVGL + Display + Animation)
 * - Core 1: 系统逻辑任务 (Sensors + Network + Commands + Business Logic)
 */
class TaskManager {
public:
    static TaskManager* getInstance();

    // 初始化任务管理器
    bool initialize();

    // 启动所有任务
    bool startTasks();

    // 发送消息到UI任务
    bool sendToUITask(const TaskMessage& msg);

    // 发送消息到系统任务
    bool sendToSystemTask(const TaskMessage& msg);

    // 获取LVGL互斥锁(在访问LVGL对象前必须获取)
    bool takeLVGLMutex(uint32_t timeout_ms = portMAX_DELAY);
    void giveLVGLMutex();

    // 获取任务句柄
    TaskHandle_t getUITaskHandle() const { return ui_task_handle_; }
    TaskHandle_t getSystemTaskHandle() const { return system_task_handle_; }

    // 任务统计信息
    void printTaskStats();

private:
    TaskManager();
    ~TaskManager();

    static TaskManager* instance_;

    // 任务句柄
    TaskHandle_t ui_task_handle_;
    TaskHandle_t system_task_handle_;

    // 消息队列
    QueueHandle_t ui_queue_;
    QueueHandle_t system_queue_;

    // LVGL互斥锁(保护LVGL对象访问)
    SemaphoreHandle_t lvgl_mutex_;

    // 任务函数(静态方法)
    static void uiTaskFunction(void* parameter);
    static void systemTaskFunction(void* parameter);

    // 禁止拷贝
    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;
};

#endif // TASK_MANAGER_H
