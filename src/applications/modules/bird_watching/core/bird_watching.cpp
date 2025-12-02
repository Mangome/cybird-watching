#include "bird_watching.h"
#include "system/logging/log_manager.h"

namespace BirdWatching {

// 全局观鸟管理器实例
BirdManager* g_birdManager = nullptr;

bool initializeBirdWatching(lv_obj_t* display_obj) {
    if (g_birdManager) {
        LOG_WARN("BIRD", "Bird watching system already initialized");
        return true;
    }

    LOG_INFO("BIRD", "Initializing Bird Watching System");

    g_birdManager = new BirdManager();
    if (!g_birdManager) {
        LOG_ERROR("BIRD", "Failed to create bird manager");
        return false;
    }

    if (!g_birdManager->initialize(display_obj)) {
        LOG_ERROR("BIRD", "Failed to initialize bird manager");
        delete g_birdManager;
        g_birdManager = nullptr;
        return false;
    }

    LOG_INFO("BIRD", "Bird Watching System initialized successfully");
    return true;
}

void updateBirdWatching() {
    if (g_birdManager) {
        g_birdManager->update();
    }
}

void processBirdTriggerRequest() {
    if (g_birdManager) {
        g_birdManager->processTriggerRequest();
    }
}

bool triggerBird() {
    if (!g_birdManager) {
        LOG_ERROR("BIRD", "Bird watching system not initialized");
        return false;
    }

    return g_birdManager->triggerBird(TRIGGER_MANUAL);
}

void showBirdStatistics() {
    if (!g_birdManager) {
        LOG_ERROR("BIRD", "Bird watching system not initialized");
        return;
    }

    g_birdManager->showStatistics();
}

void listBirds() {
    if (!g_birdManager) {
        Serial.println("Bird watching system not initialized");
        return;
    }

    // 从BirdManager获取实际的小鸟列表
    const auto& birds = g_birdManager->getAllBirds();

    Serial.println("ID     Name              Weight   Frames");
    Serial.println("----   --------------   ------   ------");

    int total_weight = 0;
    for (const auto& bird : birds) {
        // 检测每只小鸟的实际帧数
        uint8_t frame_count = g_birdManager->detectFrameCount(bird.id);

        char line[128];
        snprintf(line, sizeof(line), "%-4d   %-16s   %-6d   %-d",
                 bird.id, bird.name.c_str(), bird.weight, frame_count);
        Serial.println(line);
        total_weight += bird.weight;
    }

    Serial.println("----   --------------   ------   ------");
    char summary[128];
    snprintf(summary, sizeof(summary), "Total: %d birds, Total Weight: %d",
             (int)birds.size(), total_weight);
    Serial.println(summary);
    Serial.println();

    if (birds.empty()) {
        Serial.println("Note: No birds found. Please check bird_config.json");
    } else {
        Serial.println("Note: Loaded from bird_config.json");
    }
}

bool isBirdManagerInitialized() {
    return g_birdManager != nullptr;
}

bool isAnimationPlaying() {
    if (!g_birdManager) {
        return false;
    }
    return g_birdManager->isPlaying();
}

int getStatisticsCount() {
    if (!g_birdManager) {
        return 0;
    }
    return g_birdManager->getStatistics().getAllStats().size();
}

} // namespace BirdWatching