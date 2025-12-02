#ifndef BIRD_TYPES_H
#define BIRD_TYPES_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include "lvgl.h"

#ifdef __cplusplus

namespace BirdWatching {

// 小鸟基础信息结构（简化版）
struct BirdInfo {
    uint16_t id;               // 小鸟ID
    std::string name;          // 小鸟名称（中文）
    uint16_t weight;           // 权重（用于随机选择）

    BirdInfo() : id(0), name(""), weight(10) {}
    BirdInfo(uint16_t bird_id, const std::string& bird_name, uint16_t bird_weight = 10)
        : id(bird_id), name(bird_name), weight(bird_weight) {}
};

// 全局配置结构
struct BirdConfig {
    uint32_t auto_trigger_interval;      // 自动触发间隔（秒）
    bool enable_gesture_trigger;         // 启用手势触发
    int16_t gesture_threshold;           // 手势检测阈值
    uint32_t stats_save_interval;        // 统计数据保存间隔（秒，已废弃，固定10秒）

    BirdConfig()
        : auto_trigger_interval(900)
        , enable_gesture_trigger(true)
        , gesture_threshold(3000)
        , stats_save_interval(10) {}  // 固定为10秒
};

} // namespace BirdWatching

#endif // __cplusplus

#endif // BIRD_TYPES_H