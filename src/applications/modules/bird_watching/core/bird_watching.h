#ifndef BIRD_WATCHING_H
#define BIRD_WATCHING_H

// Bird Watching模块主头文件
// 包含所有必要的头文件和公共接口

#include "bird_types.h"
#include "bird_manager.h"
#include "bird_animation.h"
#include "bird_selector.h"
#include "bird_stats.h"

// 版本信息
#define BIRD_WATCHING_VERSION_MAJOR 1
#define BIRD_WATCHING_VERSION_MINOR 0
#define BIRD_WATCHING_VERSION_PATCH 0

// 公共宏定义
#define BIRD_WATCHING_MAX_BIRDS 20
#define BIRD_WATCHING_MAX_FRAMES_PER_BIRD 32
#define BIRD_WATCHING_DEFAULT_FPS 8

namespace BirdWatching {

// 便捷函数：初始化整个观鸟系统
bool initializeBirdWatching(lv_obj_t* display_obj = nullptr);

// 便捷函数：更新观鸟系统（在主循环中调用）
void updateBirdWatching();

// 便捷函数：处理触发请求(在UI任务中调用)
void processBirdTriggerRequest();

// 便捷函数：手动触发小鸟
bool triggerBird();

// 便捷函数：处理手势事件
void onGesture(int gesture_type);

// 便捷函数：获取观鸟统计
void showBirdStatistics();

// 便捷函数：重置观鸟统计
bool resetBirdStatistics();

// 便捷函数：列出所有可用小鸟
void listBirds();

// 便捷函数：检查系统状态
bool isBirdManagerInitialized();
bool isAnimationPlaying();
bool isStatsViewVisible();
int getStatisticsCount();

// 全局观鸟管理器实例（外部声明）
extern BirdManager* g_birdManager;

} // namespace BirdWatching

#endif // BIRD_WATCHING_H