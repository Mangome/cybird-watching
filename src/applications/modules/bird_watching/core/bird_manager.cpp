#include "bird_manager.h"
#include "system/logging/log_manager.h"
#include "drivers/sensors/imu/imu.h"
#include <cstdlib>
#include <ctime>

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
    , display_obj_(nullptr)
    , last_auto_trigger_time_(0)
    , last_stats_save_time_(0)
    , system_start_time_(0)
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
}

bool BirdManager::initialize(lv_obj_t* display_obj) {
    LOG_INFO("BIRD", "Initializing Bird Watching Manager...");

    // 保存显示对象引用
    display_obj_ = display_obj;

    // 初始化随机数生成器
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // 记录系统启动时间
    system_start_time_ = getCurrentTime();
    last_auto_trigger_time_ = system_start_time_;
    last_stats_save_time_ = system_start_time_;

    // 初始化各个子系统
    if (!initializeSubsystems(display_obj)) {
        LOG_ERROR("BIRD", "Failed to initialize bird watching subsystems");
        return false;
    }

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

    switch (gesture_type) {
        case 0: // 向前倾斜 - 触发小鸟
            LOG_DEBUG("BIRD", "Forward tilt detected, triggering bird");
            triggerBird(TRIGGER_GESTURE);
            break;

        case 1: // 向后倾斜 - 显示统计
            LOG_DEBUG("BIRD", "Backward tilt detected, showing statistics");
            showStatistics();
            break;

        case 2: // 摇动 - 随机触发
            LOG_DEBUG("BIRD", "Shake detected, random triggering");
            triggerBird(TRIGGER_GESTURE);
            break;

        default:
            LOG_DEBUG("BIRD", "Unknown gesture detected");
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
    // 初始化动画播放器
    animation_ = new BirdAnimation();
    if (!animation_ || !animation_->init(display_obj)) {
        LOG_ERROR("BIRD", "Failed to initialize bird animation system");
        return false;
    }

    // 初始化小鸟选择器
    selector_ = new BirdSelector();
    if (!selector_ || !selector_->initialize("/configs/bird_config.json")) {
        LOG_ERROR("BIRD", "Failed to initialize bird selector");
        return false;
    }

    // 初始化统计系统
    statistics_ = new BirdStatistics();
    if (!statistics_ || !statistics_->initialize()) {
        LOG_ERROR("BIRD", "Failed to initialize bird statistics");
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
            // 随机选择一个
            int random_index = std::rand() % encountered_birds.size();
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
            LOG_DEBUG("BIRD", "Statistics saved automatically");
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

uint8_t BirdManager::detectFrameCount(uint16_t bird_id) const {
    if (!animation_) {
        return 0;
    }
    return animation_->detectFrameCount(bird_id);
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
        // 新小鸟: "加新 {小鸟名字}！"
        snprintf(info_text, sizeof(info_text), 
                 "#FFFFFF 加新 # #87CEEB %s# #FFFFFF ！#", 
                 bird_name.c_str());
    } else {
        // 已见过: "{小鸟名字} 来了 {count} 次！"
        snprintf(info_text, sizeof(info_text), 
                 "#87CEEB %s# #FFFFFF 来了 # #87CEEB %d# #FFFFFF 次！#", 
                 bird_name.c_str(), count);
    }

    // 启用重新着色模式
    lv_label_set_recolor(guider_ui.scenes_bird_info_label, true);
    
    // 设置文本
    lv_label_set_text(guider_ui.scenes_bird_info_label, info_text);
    
    // 显示标签
    lv_obj_clear_flag(guider_ui.scenes_bird_info_label, LV_OBJ_FLAG_HIDDEN);
    
    // 记录日志
    char log_msg[128];
    if (is_new) {
        snprintf(log_msg, sizeof(log_msg), "Displayed bird info: %s (NEW)", bird_name.c_str());
    } else {
        snprintf(log_msg, sizeof(log_msg), "Displayed bird info: %s (x%d)", bird_name.c_str(), count);
    }
    LOG_INFO("BIRD", log_msg);
}

} // namespace BirdWatching