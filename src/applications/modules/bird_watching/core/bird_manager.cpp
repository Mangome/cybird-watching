#include "bird_manager.h"
#include "bird_utils.h"
#include "system/logging/log_manager.h"
#include "drivers/sensors/imu/imu.h"
#include "drivers/io/rgb_led/rgb_led.h"
#include "config/ui_texts.h"
#include <cstdlib>
#include "esp_system.h"

// 声明外部全局对象
extern Pixel rgb;

// 声明外部C接口，用于访问GUI对象
extern "C" {
    #include "applications/gui/core/gui_guider.h"
    extern lv_ui guider_ui;
}

namespace BirdWatching {

BirdManager::BirdManager()
    : initialized_(false)
    , first_bird_loaded_(false)
    , animation_(nullptr)
    , selector_(nullptr)
    , statistics_(nullptr)
    , stats_view_(nullptr)
    , display_obj_(nullptr)
    , last_auto_trigger_time_(0)
    , last_stats_save_time_(0)
    , system_start_time_(0)
    , bird_info_show_time_(0)
    , bird_info_visible_(false)
{
    trigger_request_.pending = false;
    trigger_request_.type = TRIGGER_AUTO;
    trigger_request_.bird_id = 0;
    trigger_request_.record_stats = true;
}

BirdManager::~BirdManager() {
    if (statistics_) {
        statistics_->saveToFile();
        delete statistics_;
    }
    if (animation_) {
        delete animation_;
    }
    if (selector_) {
        delete selector_;
    }
    if (stats_view_) {
        delete stats_view_;
    }
}

bool BirdManager::initialize(lv_obj_t* display_obj) {
    LOG_INFO("BIRD", "Initializing Bird Watching Manager...");

    // 保存显示对象引用
    display_obj_ = display_obj;

    // 记录系统启动时间
    system_start_time_ = getCurrentTime();
    last_auto_trigger_time_ = system_start_time_;
    last_stats_save_time_ = system_start_time_;

    // 初始化各个子系统
    if (!initializeSubsystems(display_obj)) {
        LOG_ERROR("BIRD", "Failed to initialize bird watching subsystems");
        return false;
    }

    // 初始化随机数生成器
    // ESP32的std::rand()质量较差，这里仅用于记录种子信息
    uint32_t raw_seed = system_start_time_;
    if (statistics_) {
        int total_encounters = statistics_->getTotalEncounters();
        raw_seed = (raw_seed * 31) + total_encounters;
    }
    
    // 使用ESP32硬件真随机数生成器(TRNG)作为种子
    // 基于射频噪声，质量远超软件LCG
    uint32_t hw_random = esp_random();
    
    // 日志输出
    char seed_msg[256];
    if (statistics_) {
        int total_encounters = statistics_->getTotalEncounters();
        snprintf(seed_msg, sizeof(seed_msg), 
                 "Random seed: context=0x%08X (millis=%u, encounters=%d), HW_RNG=0x%08X", 
                 raw_seed, system_start_time_, total_encounters, hw_random);
    } else {
        snprintf(seed_msg, sizeof(seed_msg), 
                 "Random seed: context=0x%08X (millis only), HW_RNG=0x%08X", 
                 raw_seed, hw_random);
    }
    LOG_INFO("BIRD", seed_msg);
    
    // 虽然我们主要使用esp_random()，但仍初始化rand()以备用
    std::srand(hw_random);

    initialized_ = true;
    LOG_INFO("BIRD", "Bird Watching Manager initialized successfully");
    LOG_INFO("BIRD", "Gesture trigger enabled - shake to summon birds");

    // 加载首次启动的小鸟
    loadInitialBird();

    return true;
}

void BirdManager::update() {
    if (!initialized_) {
        return;
    }

    // 注意: 此函数在System任务中调用
    // 不要直接操作LVGL对象,只设置触发请求

    // 定期保存统计数据(不涉及LVGL)
    saveStatisticsIfNeeded();
}

void BirdManager::processTriggerRequest() {
    if (!initialized_) {
        return;
    }

    // 注意: 此函数在UI任务中调用,已持有LVGL锁
    
    // 如果统计界面可见，不处理触发请求
    if (isStatsViewVisible()) {
        trigger_request_.pending = false;
        return;
    }
    
    // 检查并隐藏小鸟信息（如果超时）
    checkAndHideBirdInfo();

    // 安全地处理动画播放
    if (trigger_request_.pending) {
        bool record_stats = trigger_request_.record_stats;
        trigger_request_.pending = false;

        if (isPlaying()) {
            animation_->stop();
        }

        if (trigger_request_.bird_id > 0) {
            // 播放指定的小鸟
            playBird(trigger_request_.bird_id, record_stats);
        } else {
            // 播放随机小鸟（总是记录统计）
            playRandomBird();
        }
    }
}

bool BirdManager::triggerBird(TriggerType trigger_type) {
    if (!initialized_) {
        LOG_ERROR("BIRD_MGR", "Bird manager not initialized");
        return false;
    }

    // 设置触发请求,由UI任务处理
    trigger_request_.pending = true;
    trigger_request_.type = trigger_type;
    trigger_request_.bird_id = 0; // 随机小鸟
    trigger_request_.record_stats = true;

    return true;
}

bool BirdManager::triggerBirdById(uint16_t bird_id, TriggerType trigger_type) {
    if (!initialized_) {
        LOG_ERROR("BIRD_MGR", "Bird manager not initialized");
        return false;
    }

    if (bird_id == 0) {
        LOG_ERROR("BIRD_MGR", "Invalid bird_id: 0");
        return false;
    }

    // 验证小鸟ID是否存在
    const auto& birds = getAllBirds();
    bool bird_exists = false;
    for (const auto& bird : birds) {
        if (bird.id == bird_id) {
            bird_exists = true;
            break;
        }
    }

    if (!bird_exists) {
        LOG_ERROR("BIRD_MGR", String("Bird ID not found: ") + String(bird_id));
        return false;
    }

    // 设置触发请求,由UI任务处理
    trigger_request_.pending = true;
    trigger_request_.type = trigger_type;
    trigger_request_.bird_id = bird_id; // 指定小鸟
    trigger_request_.record_stats = true;

    LOG_INFO("BIRD_MGR", String("Trigger request set for bird ID: ") + String(bird_id));
    return true;
}

bool BirdManager::playBirdWithoutRecording(uint16_t bird_id) {
    if (!initialized_) {
        LOG_ERROR("BIRD_MGR", "Bird manager not initialized");
        return false;
    }

    if (bird_id == 0) {
        LOG_ERROR("BIRD_MGR", "Invalid bird_id");
        return false;
    }

    // 设置触发请求,由UI任务处理（不记录统计）
    trigger_request_.pending = true;
    trigger_request_.type = TRIGGER_AUTO;
    trigger_request_.bird_id = bird_id;
    trigger_request_.record_stats = false;

    return true;
}

void BirdManager::onGestureEvent(int gesture_type) {
    if (!config_.enable_gesture_trigger) {
        return;
    }

    static unsigned long last_tilt_trigger_time = 0; // 左右倾触发新鸟的CD
    unsigned long current_time = getCurrentTime();

    LOG_DEBUG("BIRD", (String("Gesture event received: ") + String(gesture_type) + 
              String(", Stats view visible: ") + String(isStatsViewVisible() ? "yes" : "no")).c_str());

    switch (gesture_type) {
        case GESTURE_FORWARD_HOLD: // 前倾保持3秒 - 进入数据界面
            LOG_INFO("BIRD", "Forward hold 1s detected, showing stats view");
            showStatsView();
            rgb.flashGreen(100); // 绿光闪一下
            break;

        case GESTURE_BACKWARD_HOLD: // 后倾保持3秒 - 退出数据界面
            if (isStatsViewVisible()) {
                LOG_INFO("BIRD", "Backward hold 1s detected, hiding stats view");
                hideStatsView();
                rgb.flashGreen(100); // 绿光闪一下
            } else {
                LOG_DEBUG("BIRD", "Backward hold ignored, not in stats view");
            }
            break;

        case GESTURE_LEFT_TILT: // 左倾
        case GESTURE_RIGHT_TILT: // 右倾
            if (isStatsViewVisible()) {
                // 统计界面中：切换页面（无CD限制）
                if (gesture_type == GESTURE_LEFT_TILT) {
                    LOG_DEBUG("BIRD", "Left tilt in stats view, previous page");
                    statsViewPreviousPage();
                } else {
                    LOG_DEBUG("BIRD", "Right tilt in stats view, next page");
                    statsViewNextPage();
                }
                rgb.flashBlue(100); // 蓝光闪一下
            } else {
                // 主界面中：触发新鸟（10秒CD）
                unsigned long time_since_last_trigger = current_time - last_tilt_trigger_time;
                if (time_since_last_trigger >= 10000) {
                    LOG_DEBUG("BIRD", (String(gesture_type == GESTURE_LEFT_TILT ? "Left" : "Right") + 
                              String(" tilt in main view, triggering bird")).c_str());
                    triggerBird(TRIGGER_GESTURE);
                    last_tilt_trigger_time = current_time;
                    rgb.flashBlue(100); // 蓝光闪一下
                } else {
                    unsigned long remaining = 10000 - time_since_last_trigger;
                    LOG_DEBUG("BIRD", (String("Tilt ignored, CD active: ") + String(remaining) + 
                              String("ms remaining")).c_str());
                }
            }
            break;

        default:
            break;
    }
}

void BirdManager::showStatistics() {
    if (!statistics_) {
        LOG_ERROR("BIRD", "Statistics system not available");
        return;
    }

    statistics_->printStats();
}

void BirdManager::setConfig(const BirdConfig& config) {
    config_ = config;
    LOG_INFO("BIRD", "Bird manager configuration updated");
}

void BirdManager::saveConfig() {
    // TODO: 实现配置保存
    LOG_INFO("BIRD", "Configuration saving not yet implemented");
}

bool BirdManager::initializeSubsystems(lv_obj_t* display_obj) {
    // display_obj现在是scenes对象
    // 需要从scenes中获取scenes_canvas作为动画显示对象
    // guider_ui已在文件顶部通过extern "C"声明
    lv_obj_t* canvas_obj = guider_ui.scenes_canvas;
    
    // 初始化动画播放器（使用scenes_canvas）
    animation_ = new BirdAnimation();
    if (!animation_ || !animation_->init(canvas_obj)) {
        LOG_ERROR("BIRD", "Failed to initialize bird animation system");
        return false;
    }

    // 初始化小鸟选择器
    selector_ = new BirdSelector();
    if (!selector_ || !selector_->initialize("/configs/bird_config.csv")) {
        LOG_ERROR("BIRD", "Failed to initialize bird selector");
        return false;
    }

    // 初始化统计系统
    statistics_ = new BirdStatistics();
    if (!statistics_ || !statistics_->initialize()) {
        LOG_ERROR("BIRD", "Failed to initialize bird statistics");
        return false;
    }

    // 初始化统计界面（使用scenes作为父对象）
    stats_view_ = new StatsView();
    if (!stats_view_ || !stats_view_->initialize(display_obj, statistics_, selector_)) {
        LOG_ERROR("BIRD", "Failed to initialize stats view");
        return false;
    }

    LOG_INFO("BIRD", "All subsystems initialized successfully");
    return true;
}

void BirdManager::handleAutoTrigger() {
    // 已移除自动定时触发逻辑，改为仅通过IMU摇晃手势触发
    // 此函数保留以便将来可能需要的功能扩展
}

bool BirdManager::playRandomBird() {
    if (!selector_ || !animation_) {
        LOG_ERROR("BIRD", "Bird selector or animation not available");
        return false;
    }

    // 随机选择一只小鸟
    BirdInfo bird = selector_->getRandomBird();
    if (bird.id == 0) {
        LOG_ERROR("BIRD", "Failed to select random bird");
        return false;
    }

    return playBird(bird.id, true);
}

bool BirdManager::playBird(uint16_t bird_id, bool record_stats) {
    if (!selector_ || !animation_) {
        LOG_ERROR("BIRD", "Bird selector or animation not available");
        return false;
    }

    // 从选择器获取小鸟信息
    const std::vector<BirdInfo>& all_birds = selector_->getAllBirds();
    const BirdInfo* bird_info = nullptr;
    
    for (const auto& bird : all_birds) {
        if (bird.id == bird_id) {
            bird_info = &bird;
            break;
        }
    }

    if (!bird_info) {
        LOG_ERROR("BIRD", (String("Bird not found with ID: ") + String(bird_id)).c_str());
        return false;
    }

    // 检查是否为新小鸟（在记录统计之前检查）
    bool is_new_bird = false;
    if (record_stats && statistics_) {
        is_new_bird = (statistics_->getEncounterCount(bird_id) == 0);
    }

    // 加载小鸟动画
    if (!animation_->loadBird(*bird_info)) {
        LOG_ERROR("BIRD", "Failed to load bird");
        return false;
    }

    // 播放动画（循环播放）
    animation_->startLoop();

    // 记录统计数据（如果需要）
    if (record_stats && statistics_) {
        statistics_->recordEncounter(bird_id);
    }

    // 显示小鸟信息
    if (record_stats) {
        showBirdInfo(bird_id, bird_info->name, is_new_bird);
    }

    LOG_INFO("BIRD", (String("Playing bird animation (ID: ") + String(bird_id) + 
             ", record: " + String(record_stats ? "yes" : "no") + ")").c_str());
    return true;
}

void BirdManager::loadInitialBird() {
    if (first_bird_loaded_) {
        return;
    }

    if (!statistics_ || !selector_) {
        LOG_ERROR("BIRD", "Statistics or selector not available");
        return;
    }

    // 检查是否有历史统计数据
    if (statistics_->hasHistoricalData()) {
        // 有历史数据：随机选择一个已经遇见过的小鸟，不计数
        std::vector<uint16_t> encountered_birds = statistics_->getEncounteredBirdIds();
        if (!encountered_birds.empty()) {
            // 使用硬件随机数选择
            uint32_t random_index = esp_random() % encountered_birds.size();
            uint16_t bird_id = encountered_birds[random_index];
            
            LOG_INFO("BIRD", (String("Loading initial bird from history (ID: ") + String(bird_id) + ")").c_str());
            playBird(bird_id, false); // 不记录统计
        }
    } else {
        // 没有历史数据：正常触发一次，计数
        LOG_INFO("BIRD", "No historical data, triggering first bird with counting");
        triggerBird(TRIGGER_AUTO);
    }

    first_bird_loaded_ = true;
    
    // 注意: 不在这里调用lv_hide_logo()，因为此时可能没有LVGL互斥锁
    // UI任务会在检测到动画播放时自动隐藏logo
}

void BirdManager::updateGestureDetection() {
    // 手势检测已集成到TaskManager的系统任务中
    // 通过IMU.detectGesture()实时检测并触发相应事件
}

void BirdManager::handleGesture(GestureType gesture) {
    switch (gesture) {
        case GESTURE_FORWARD_TILT:
            LOG_INFO("BIRD", "Forward tilt detected, triggering bird");
            triggerBird(TRIGGER_GESTURE);
            break;

        case GESTURE_BACKWARD_TILT:
            LOG_INFO("BIRD", "Backward tilt detected, showing statistics");
            showStatistics();
            break;

        default:
            break;
    }
}

void BirdManager::saveStatisticsIfNeeded() {
    if (!statistics_) {
        return;
    }

    uint32_t current_time = getCurrentTime();
    uint32_t time_since_last_save = current_time - last_stats_save_time_;

    // 每10秒保存一次（10000毫秒）
    if (time_since_last_save >= 10000) {
        if (statistics_->saveToFile()) {
            last_stats_save_time_ = current_time;
            // LOG_DEBUG("BIRD", "Statistics saved automatically");
        }
    }
}

uint32_t BirdManager::getCurrentTime() const {
    // 使用Arduino的millis()函数
    return millis();
}

uint32_t BirdManager::getNextAutoTriggerTime() const {
    return last_auto_trigger_time_ + (config_.auto_trigger_interval * 1000);
}

const std::vector<BirdInfo>& BirdManager::getAllBirds() const {
    if (!selector_) {
        static std::vector<BirdInfo> empty;
        return empty;
    }
    return selector_->getAllBirds();
}

void BirdManager::checkAndHideBirdInfo() {
    // 如果信息可见且已超过5秒，则隐藏
    if (bird_info_visible_) {
        uint32_t current_time = getCurrentTime();
        if (current_time - bird_info_show_time_ >= 5000) {
            hideBirdInfo();
        }
    }
}

void BirdManager::hideBirdInfo() {
    if (!guider_ui.scenes_bird_info_label) {
        return;
    }

    // 隐藏标签
    lv_obj_add_flag(guider_ui.scenes_bird_info_label, LV_OBJ_FLAG_HIDDEN);
    bird_info_visible_ = false;
}

void BirdManager::showBirdInfo(uint16_t bird_id, const std::string& bird_name, bool is_new) {
    if (!guider_ui.scenes_bird_info_label) {
        LOG_ERROR("BIRD", "Bird info label not available");
        return;
    }

    // 获取当前次数（已经记录过了，所以需要获取最新的count）
    int count = statistics_ ? statistics_->getEncounterCount(bird_id) : 0;

    // 构建信息文本
    // 使用 Noto Sans SC 字体显示中文
    // LVGL颜色格式: #RRGGBB text#
    char info_text[256];
    if (is_new) {
        // 新小鸟: "加新{小鸟名字}！"
        snprintf(info_text, sizeof(info_text), 
                 "#FFFFFF %s##87CEEB %s##FFFFFF %s#", 
                 UITexts::BirdInfo::NEW_BIRD_PREFIX,
                 bird_name.c_str(),
                 UITexts::BirdInfo::NEW_BIRD_SUFFIX);
    } else {
        // 已见过: "{小鸟名字}来了{count}次！"
        snprintf(info_text, sizeof(info_text), 
                 "#87CEEB %s##FFFFFF %s##87CEEB %d##FFFFFF %s#", 
                 bird_name.c_str(),
                 UITexts::BirdInfo::VISIT_COUNT_MIDDLE,
                 count,
                 UITexts::BirdInfo::VISIT_COUNT_SUFFIX);
    }

    // 启用重新着色模式
    lv_label_set_recolor(guider_ui.scenes_bird_info_label, true);
    
    // 设置文本
    lv_label_set_text(guider_ui.scenes_bird_info_label, info_text);
    
    // 显示标签
    lv_obj_clear_flag(guider_ui.scenes_bird_info_label, LV_OBJ_FLAG_HIDDEN);
    
    // 记录显示时间
    bird_info_show_time_ = getCurrentTime();
    bird_info_visible_ = true;
    
    // 记录日志
    char log_msg[128];
    if (is_new) {
        snprintf(log_msg, sizeof(log_msg), "Displayed bird info: %s (NEW)", bird_name.c_str());
    } else {
        snprintf(log_msg, sizeof(log_msg), "Displayed bird info: %s (x%d)", bird_name.c_str(), count);
    }
    LOG_INFO("BIRD", log_msg);
}

void BirdManager::showStatsView() {
    if (!stats_view_) {
        LOG_ERROR("BIRD", "Stats view not available");
        return;
    }

    // 停止动画
    if (animation_ && isPlaying()) {
        animation_->stop();
    }

    // 隐藏小鸟信息
    hideBirdInfo();

    // 显示统计界面
    stats_view_->show();
    LOG_INFO("BIRD", "Stats view shown");
}

void BirdManager::hideStatsView() {
    if (!stats_view_) {
        return;
    }

    stats_view_->hide();
    LOG_INFO("BIRD", "Stats view hidden");
    
    // 退出统计界面后，显示一个小鸟
    // 逻辑与初始化时相同：有历史数据则随机选择已记录的小鸟（不计数），没有则触发新鸟（计数）
    if (statistics_ && statistics_->hasHistoricalData()) {
        // 有历史数据：随机选择一个已经遇见过的小鸟，不计数
        std::vector<uint16_t> encountered_birds = statistics_->getEncounteredBirdIds();
        if (!encountered_birds.empty()) {
            // 使用硬件随机数选择
            uint32_t random_index = esp_random() % encountered_birds.size();
            uint16_t bird_id = encountered_birds[random_index];
            
            LOG_INFO("BIRD", (String("Displaying random encountered bird (ID: ") + String(bird_id) + ")").c_str());
            playBird(bird_id, false); // 不记录统计
        }
    } else {
        // 没有历史数据：触发一次新鸟，计数
        LOG_INFO("BIRD", "No historical data, triggering new bird");
        triggerBird(TRIGGER_AUTO);
    }
}

bool BirdManager::isStatsViewVisible() const {
    return stats_view_ ? stats_view_->isVisible() : false;
}

void BirdManager::statsViewPreviousPage() {
    if (stats_view_) {
        stats_view_->previousPage();
    }
}

void BirdManager::statsViewNextPage() {
    if (stats_view_) {
        stats_view_->nextPage();
    }
}

} // namespace BirdWatching