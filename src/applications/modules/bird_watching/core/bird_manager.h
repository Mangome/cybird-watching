#ifndef BIRD_MANAGER_H
#define BIRD_MANAGER_H

#include "bird_animation.h"
#include "bird_selector.h"
#include "bird_stats.h"
#include "bird_types.h"
#include "drivers/sensors/imu/imu.h"
#include <string>
#include <vector>

namespace BirdWatching {

// 触发类型枚举
enum TriggerType {
    TRIGGER_AUTO = 0,       // 自动触发
    TRIGGER_MANUAL,         // 手动触发
    TRIGGER_GESTURE         // 手势触发
};

// 触发请求结构(用于任务间通信)
struct BirdTriggerRequest {
    bool pending;
    TriggerType type;
    uint16_t bird_id;  // 0表示随机
    bool record_stats; // 是否记录统计
};

class BirdManager {
public:
    BirdManager();
    ~BirdManager();

    // 初始化整个观鸟系统
    bool initialize(lv_obj_t* display_obj = nullptr);

    // 系统更新（在主循环中调用）
    void update();

    // 处理触发请求(在UI任务中调用)
    void processTriggerRequest();

    // 手动触发小鸟出现(设置触发请求)
    bool triggerBird(TriggerType trigger_type = TRIGGER_MANUAL);
    
    // 播放指定小鸟（不记录统计）
    bool playBirdWithoutRecording(uint16_t bird_id);

    // 处理手势事件
    void onGestureEvent(int gesture_type);

    // 显示统计信息
    void showStatistics();

    // 获取系统状态
    bool isInitialized() const { return initialized_; }
    bool isPlaying() const { return animation_ ? animation_->isPlaying() : false; }

    // 配置管理
    BirdConfig& getConfig() { return config_; }
    void setConfig(const BirdConfig& config);
    void saveConfig();

    // 获取统计信息
    const BirdStatistics& getStatistics() const { return *statistics_; }

    // 获取小鸟列表
    const std::vector<BirdInfo>& getAllBirds() const;

    // 检测小鸟的帧数
    uint8_t detectFrameCount(uint16_t bird_id) const;

private:
    bool initialized_;                           // 初始化状态
    bool first_bird_loaded_;                     // 首次小鸟是否已加载
    BirdConfig config_;                          // 全局配置
    BirdAnimation* animation_;                   // 动画播放器
    BirdSelector* selector_;                     // 小鸟选择器
    BirdStatistics* statistics_;                 // 统计系统

    uint32_t last_auto_trigger_time_;            // 上次自动触发时间
    uint32_t last_stats_save_time_;              // 上次统计数据保存时间
    uint32_t system_start_time_;                 // 系统启动时间

    // 触发请求(用于跨任务通信)
    BirdTriggerRequest trigger_request_;

    // 初始化各个子系统
    bool initializeSubsystems(lv_obj_t* display_obj);
    
    // 加载首次启动的小鸟
    void loadInitialBird();

    // 处理自动触发
    void handleAutoTrigger();

    // 播放随机小鸟
    bool playRandomBird();
    
    // 播放指定小鸟（内部使用，可选择是否记录统计）
    bool playBird(uint16_t bird_id, bool record_stats = true);

    // 更新手势检测
    void updateGestureDetection();

    // 保存统计数据
    void saveStatisticsIfNeeded();

    // 获取当前时间（毫秒）
    uint32_t getCurrentTime() const;

    // 计算下次自动触发时间
    uint32_t getNextAutoTriggerTime() const;

    // 处理手势事件
    void handleGesture(GestureType gesture);
};

} // namespace BirdWatching

#endif // BIRD_MANAGER_H