#include "bird_watching.h"

#include "bird_utils.h"
#include "system/logging/log_manager.h"

namespace BirdWatching
{

// 全局观鸟管理器实例
BirdManager* g_birdManager = nullptr;

bool initializeBirdWatching(lv_obj_t* display_obj)
{
    if(g_birdManager) {
        LOG_WARN("BIRD", "Bird watching system already initialized");
        return true;
    }

    LOG_INFO("BIRD", "Initializing Bird Watching System");

    g_birdManager = new BirdManager();
    if(!g_birdManager) {
        LOG_ERROR("BIRD", "Failed to create bird manager");
        return false;
    }

    if(!g_birdManager->initialize(display_obj)) {
        LOG_ERROR("BIRD", "Failed to initialize bird manager");
        delete g_birdManager;
        g_birdManager = nullptr;
        return false;
    }

    LOG_INFO("BIRD", "Bird Watching System initialized successfully");
    return true;
}

void updateBirdWatching()
{
    if(g_birdManager) {
        g_birdManager->update();
    }
}

void processBirdTriggerRequest()
{
    if(g_birdManager) {
        g_birdManager->processTriggerRequest();
    }
}

bool triggerBird(uint16_t bird_id)
{
    if(!g_birdManager) {
        LOG_ERROR("BIRD", "Bird watching system not initialized");
        return false;
    }

    if(bird_id == 0) {
        // 随机触发
        return g_birdManager->triggerBird(TRIGGER_MANUAL);
    } else {
        // 触发指定小鸟
        return g_birdManager->triggerBirdById(bird_id, TRIGGER_MANUAL);
    }
}

void onGesture(int gesture_type)
{
    if(!g_birdManager) {
        LOG_ERROR("BIRD", "Bird watching system not initialized");
        return;
    }

    g_birdManager->onGestureEvent(gesture_type);
}

void showBirdStatistics()
{
    if(!g_birdManager) {
        LOG_ERROR("BIRD", "Bird watching system not initialized");
        return;
    }

    g_birdManager->showStatistics();
}

bool resetBirdStatistics()
{
    if(!g_birdManager) {
        LOG_ERROR("BIRD", "Bird watching system not initialized");
        return false;
    }

    // 获取统计对象并重置
    BirdStatistics& stats = const_cast<BirdStatistics&>(g_birdManager->getStatistics());
    stats.resetStats();

    // 立即保存到文件
    bool saved = stats.saveToFile();
    if(saved) {
        LOG_INFO("BIRD", "Statistics reset and saved successfully");
    } else {
        LOG_ERROR("BIRD", "Statistics reset but failed to save to file");
    }

    return saved;
}

void listBirds()
{
    if(!g_birdManager) {
        Serial.println("Bird watching system not initialized");
        return;
    }

    // 从BirdManager获取实际的小鸟列表
    const auto& birds = g_birdManager->getAllBirds();

    Serial.println("ID     Name              Weight   Frames");
    Serial.println("----   --------------   ------   ------");

    int total_weight = 0;
    for(auto& bird : birds) {
        // 使用缓存的帧数，如果未缓存则检测
        if(bird.frame_count == 0) {
            bird.frame_count = BirdWatching::Utils::detectFrameCount(bird.id);
        }

        char line[128];
        snprintf(line, sizeof(line), "%-4d   %-16s   %-6d   %-d", bird.id, bird.name.c_str(), bird.weight,
                 bird.frame_count);
        Serial.println(line);
        total_weight += bird.weight;

        // 每输出一只小鸟，喂一次狗
        yield();
    }

    Serial.println("----   --------------   ------   ------");
    char summary[128];
    snprintf(summary, sizeof(summary), "Total: %d birds, Total Weight: %d", (int)birds.size(), total_weight);
    Serial.println(summary);
    Serial.println();

    if(birds.empty()) {
        Serial.println("Note: No birds found. Please check bird_config.csv");
    } else {
        Serial.println("Note: Loaded from bird_config.csv");
    }
}

bool isBirdManagerInitialized()
{
    return g_birdManager != nullptr;
}

bool isAnimationPlaying()
{
    if(!g_birdManager) {
        return false;
    }
    return g_birdManager->isPlaying();
}

bool isStatsViewVisible()
{
    if(!g_birdManager) {
        return false;
    }
    return g_birdManager->isStatsViewVisible();
}

int getStatisticsCount()
{
    if(!g_birdManager) {
        return 0;
    }
    return g_birdManager->getStatistics().getEncounteredBirdIds().size();
}

}  // namespace BirdWatching