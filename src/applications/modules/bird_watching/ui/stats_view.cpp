#include "stats_view.h"
#include "../core/bird_stats.h"
#include "../core/bird_selector.h"
#include "../core/bird_types.h"
#include "system/logging/log_manager.h"
#include "config/guider_fonts.h"

namespace BirdWatching {

StatsView::StatsView()
    : visible_(false)
    , current_page_(0)
    , total_pages_(0)
    , container_(nullptr)
    , title_label_(nullptr)
    , prev_label_(nullptr)
    , next_label_(nullptr)
    , page_indicator_(nullptr)
    , statistics_(nullptr)
    , selector_(nullptr)
{
    for (int i = 0; i < 5; i++) {
        bird_labels_[i] = nullptr;
    }
}

StatsView::~StatsView() {
    // LVGL对象会自动清理
}

bool StatsView::initialize(lv_obj_t* parent, BirdStatistics* stats, BirdSelector* selector) {
    if (!parent || !stats || !selector) {
        LOG_ERROR("STATS_VIEW", "Invalid parameters");
        return false;
    }

    statistics_ = stats;
    selector_ = selector;

    createUI(parent);
    
    LOG_INFO("STATS_VIEW", "Stats view initialized");
    return true;
}

void StatsView::createUI(lv_obj_t* parent) {
    // 创建容器（覆盖整个屏幕）
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, 240, 240);
    lv_obj_set_pos(container_, 0, 0);
    lv_obj_set_style_bg_color(container_, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(container_, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(container_, 0, LV_PART_MAIN);
    lv_obj_clear_flag(container_, LV_OBJ_FLAG_SCROLLABLE); // 禁用滚动
    lv_obj_add_flag(container_, LV_OBJ_FLAG_HIDDEN); // 默认隐藏
    
    // 将容器移到最顶层
    lv_obj_move_foreground(container_);

    // 创建标题："观鸟统计"（居中显示）
    title_label_ = lv_label_create(container_);
    lv_label_set_text(title_label_, "观鸟统计");
    lv_obj_set_style_text_color(title_label_, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(title_label_, &lv_font_notosanssc_16, LV_PART_MAIN); // 改用16号字体
    lv_obj_align(title_label_, LV_ALIGN_TOP_MID, 0, 10);

    // 创建5行小鸟信息标签
    for (int i = 0; i < 5; i++) {
        bird_labels_[i] = lv_label_create(container_);
        lv_label_set_text(bird_labels_[i], "");
        lv_obj_set_style_text_color(bird_labels_[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_text_font(bird_labels_[i], &lv_font_notosanssc_16, LV_PART_MAIN);
        lv_obj_align(bird_labels_[i], LV_ALIGN_TOP_LEFT, 10, 40 + i * 30);
        lv_label_set_recolor(bird_labels_[i], true); // 启用颜色标记
    }

    // 创建\"上一页\"标签（左对齐）
    prev_label_ = lv_label_create(container_);
    lv_label_set_text(prev_label_, "上一页");
    lv_obj_set_style_text_color(prev_label_, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_set_style_text_font(prev_label_, &lv_font_notosanssc_16, LV_PART_MAIN);
    lv_obj_align(prev_label_, LV_ALIGN_BOTTOM_LEFT, 10, -10);

    // 创建\"下一页\"标签（右对齐）
    next_label_ = lv_label_create(container_);
    lv_label_set_text(next_label_, "下一页");
    lv_obj_set_style_text_color(next_label_, lv_color_hex(0x888888), LV_PART_MAIN);
    lv_obj_set_style_text_font(next_label_, &lv_font_notosanssc_16, LV_PART_MAIN);
    lv_obj_align(next_label_, LV_ALIGN_BOTTOM_RIGHT, -10, -10);

    // 创建页码标识标签（底部居中）
    page_indicator_ = lv_label_create(container_);
    lv_label_set_text(page_indicator_, "1/1");
    lv_obj_set_style_text_color(page_indicator_, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(page_indicator_, &lv_font_notosanssc_16, LV_PART_MAIN);
    lv_obj_align(page_indicator_, LV_ALIGN_BOTTOM_MID, 0, -10);
}

void StatsView::show() {
    if (!container_) {
        LOG_ERROR("STATS_VIEW", "Container not initialized");
        return;
    }

    // 重置到第一页
    current_page_ = 0;
    
    // 更新内容
    update();
    
    // 移到最顶层
    lv_obj_move_foreground(container_);
    
    // 显示容器
    lv_obj_clear_flag(container_, LV_OBJ_FLAG_HIDDEN);
    visible_ = true;
    
    LOG_INFO("STATS_VIEW", "Stats view shown");
}

void StatsView::hide() {
    if (!container_) {
        return;
    }

    lv_obj_add_flag(container_, LV_OBJ_FLAG_HIDDEN);
    visible_ = false;
    
    LOG_INFO("STATS_VIEW", "Stats view hidden");
}

void StatsView::previousPage() {
    if (current_page_ > 0) {
        current_page_--;
        update();
        LOG_INFO("STATS_VIEW", (String("Previous page: ") + String(current_page_)).c_str());
    }
}

void StatsView::nextPage() {
    if (current_page_ < total_pages_ - 1) {
        current_page_++;
        update();
        LOG_INFO("STATS_VIEW", (String("Next page: ") + String(current_page_)).c_str());
    }
}

void StatsView::update() {
    if (!statistics_ || !selector_) {
        LOG_ERROR("STATS_VIEW", "Statistics or selector not available");
        return;
    }

    updateBirdList();
}

void StatsView::updateBirdList() {
    // 获取所有小鸟信息
    const std::vector<BirdInfo>& all_birds = selector_->getAllBirds();
    
    // 计算总页数（每页5个，从1001开始）
    int total_birds = all_birds.size();
    total_pages_ = (total_birds + 4) / 5; // 向上取整
    
    if (total_pages_ == 0) {
        total_pages_ = 1;
    }

    // 计算当前页显示的小鸟范围
    int start_index = current_page_ * 5;
    int end_index = start_index + 5;
    if (end_index > total_birds) {
        end_index = total_birds;
    }

    // 更新5行小鸟信息
    for (int i = 0; i < 5; i++) {
        int bird_index = start_index + i;
        
        if (bird_index < total_birds) {
            const BirdInfo& bird = all_birds[bird_index];
            int count = statistics_->getEncounterCount(bird.id);
            
            char text[128];
            if (count > 0) {
                // 已解锁：显示 "id. 名字 x count"
                // 格式：蓝色ID和点号，白色名字，蓝色"x"，白色次数
                snprintf(text, sizeof(text), 
                         "#87CEEB %d.##FFFFFF %s ##87CEEB x ##FFFFFF %d#",
                         bird.id, bird.name.c_str(), count);
            } else {
                // 未解锁：显示 "？？？"
                snprintf(text, sizeof(text), "#666666 ？？？#");
            }
            
            lv_label_set_text(bird_labels_[i], text);
            lv_obj_clear_flag(bird_labels_[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            // 超出范围，隐藏标签
            lv_label_set_text(bird_labels_[i], "");
            lv_obj_add_flag(bird_labels_[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    // 更新上一页/下一页标签颜色
    if (current_page_ > 0) {
        lv_obj_set_style_text_color(prev_label_, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    } else {
        lv_obj_set_style_text_color(prev_label_, lv_color_hex(0x666666), LV_PART_MAIN);
    }

    if (current_page_ < total_pages_ - 1) {
        lv_obj_set_style_text_color(next_label_, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    } else {
        lv_obj_set_style_text_color(next_label_, lv_color_hex(0x666666), LV_PART_MAIN);
    }

    // 更新页码标识
    char page_text[16];
    snprintf(page_text, sizeof(page_text), "%d/%d", current_page_ + 1, total_pages_);
    lv_label_set_text(page_indicator_, page_text);
}

std::string StatsView::getBirdName(uint16_t bird_id) const {
    if (!selector_) {
        return "Unknown";
    }

    const std::vector<BirdInfo>& all_birds = selector_->getAllBirds();
    for (const auto& bird : all_birds) {
        if (bird.id == bird_id) {
            return bird.name;
        }
    }

    return "Unknown";
}

} // namespace BirdWatching
