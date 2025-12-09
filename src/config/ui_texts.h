#ifndef UI_TEXTS_H
#define UI_TEXTS_H

#include <Arduino.h>

/**
 * UI文本配置
 * 所有需要在屏幕UI上展示的文本集中在这里管理
 * 注意：不包括日志输出和串口调试信息
 */
namespace UITexts {

// ============================================
// 统计界面文本
// ============================================
namespace StatsView {
    constexpr const char* TITLE = "观鸟统计";
    constexpr const char* PREV_PAGE = "上一页";
    constexpr const char* NEXT_PAGE = "下一页";
    constexpr const char* UNKNOWN_BIRD = "？？？";
}

// ============================================
// 小鸟信息文本
// ============================================
namespace BirdInfo {
    constexpr const char* NEW_BIRD_PREFIX = "加新";
    constexpr const char* NEW_BIRD_SUFFIX = "！";
    constexpr const char* VISIT_COUNT_MIDDLE = "来了";
    constexpr const char* VISIT_COUNT_SUFFIX = "次！";
    constexpr const char* UNKNOWN = "Unknown";
}

} // namespace UITexts

#endif // UI_TEXTS_H
