#ifndef STATS_VIEW_H
#define STATS_VIEW_H

#include "lvgl.h"
#include <vector>
#include <string>
#include <cstdint>

namespace BirdWatching {

// 前向声明
class BirdStatistics;
class BirdSelector;

class StatsView {
public:
    StatsView();
    ~StatsView();

    // 初始化统计界面
    bool initialize(lv_obj_t* parent, BirdStatistics* stats, BirdSelector* selector);

    // 显示统计界面
    void show();

    // 隐藏统计界面
    void hide();

    // 检查是否正在显示
    bool isVisible() const { return visible_; }

    // 切换到上一页
    void previousPage();

    // 切换到下一页
    void nextPage();

    // 更新界面内容
    void update();

private:
    bool visible_;
    int current_page_;
    int total_pages_;
    
    // LVGL 对象
    lv_obj_t* container_;          // 容器对象
    lv_obj_t* title_label_;        // 标题：\"观鸟统计\"
    lv_obj_t* bird_labels_[5];     // 5行小鸟信息
    lv_obj_t* prev_label_;         // \"上一页\"标签
    lv_obj_t* next_label_;         // \"下一页\"标签
    lv_obj_t* page_indicator_;     // 页码标识标签
    
    // 数据引用
    BirdStatistics* statistics_;
    BirdSelector* selector_;
    
    // 内部方法
    void createUI(lv_obj_t* parent);
    void updateBirdList();
    std::string getBirdName(uint16_t bird_id) const;
};

} // namespace BirdWatching

#endif // STATS_VIEW_H
