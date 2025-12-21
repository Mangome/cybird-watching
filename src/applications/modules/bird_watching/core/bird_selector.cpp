#include "bird_selector.h"
#include "bird_utils.h"
#include "system/logging/log_manager.h"
#include "hal/sd_interface.h"
#include "esp_system.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <vector>

// CSV解析辅助函数
namespace {
    // 解析CSV行 - 跳过引号处理
    void parseCSVLine(const char* line, int& id, std::string& name, int& weight) {
        id = 0;
        name.clear();
        weight = 0;

        const char* ptr = line;

        // 解析ID
        id = atoi(ptr);

        // 跳过到逗号后的name字段
        ptr = strchr(ptr, ',');
        if (!ptr) return;
        ptr++; // 跳过逗号

        // 解析name（处理可能的引号）
        if (*ptr == '"') {
            ptr++; // 跳过开始引号
            while (*ptr && *ptr != '"') {
                name += *ptr++;
            }
            if (*ptr == '"') ptr++; // 跳过结束引号
        } else {
            while (*ptr && *ptr != ',') {
                name += *ptr++;
            }
        }

        // 跳过到逗号后的weight字段
        ptr = strchr(ptr, ',');
        if (!ptr) return;
        ptr++; // 跳过逗号

        // 解析weight
        weight = atoi(ptr);
    }

    // 去除字符串首尾空白字符
    std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }
}

namespace BirdWatching {

BirdSelector::BirdSelector() : total_weight_(0) {
}

BirdSelector::~BirdSelector() {
}

bool BirdSelector::initialize(const std::string& config_path) {
    birds_.clear();
    total_weight_ = 0;

    // 尝试加载配置文件
    if (!loadBirdConfig(config_path)) {
        LOG_WARN("SELECTOR", "Failed to load bird config, using defaults");

        // 使用默认的小鸟列表（简化版）
        birds_.emplace_back(1001, "普通翠鸟", 50);
        total_weight_ += 50;

        birds_.emplace_back(1002, "叉尾太阳鸟", 30);
        total_weight_ += 30;
    }

    LOG_INFO("SELECTOR", "Bird selector initialized");

    return !birds_.empty();
}

BirdInfo BirdSelector::getRandomBird() {
    if (birds_.empty()) {
        LOG_ERROR("BIRD", "No birds available for selection");
        return BirdInfo();
    }

    // 使用ESP32硬件真随机数生成器(TRNG)
    // 基于射频噪声，质量远超std::rand()
    uint32_t hw_random = esp_random();
    int random_weight = hw_random % total_weight_;

    int current_weight = 0;
    for (const auto& bird : birds_) {
        current_weight += bird.weight;
        if (random_weight < current_weight) {
            LOG_DEBUG("SELECTOR", "Bird selected by weight");
            return bird;
        }
    }

    // 如果由于某种原因没有选中，返回第一只鸟
    LOG_WARN("SELECTOR", "Random selection fallback, returning first bird");
    return birds_[0];
}

const BirdInfo* BirdSelector::findBird(const std::string& name) const {
    for (const auto& bird : birds_) {
        if (bird.name == name) {
            return &bird;
        }
    }
    return nullptr;
}

bool BirdSelector::reloadConfig() {
    return initialize("S:/configs/bird_config.csv");
}

bool BirdSelector::loadBirdConfig(const std::string& config_path) {
    std::string path_msg = "Attempting to load bird config from: " + config_path;
    LOG_INFO("SELECTOR", path_msg.c_str());


    // 直接使用正确路径
    fs::FS& fs = HAL::SDInterface::getFS();
    File file = fs.open("/configs/bird_config.csv", "r");

    if (!file) {
        LOG_ERROR("SELECTOR", "Cannot open bird config file: /configs/bird_config.csv");
        return false;
    }

    LOG_INFO("SELECTOR", "Successfully opened bird config file");

    // 读取文件内容
    long file_size = file.size();

    char size_msg[64];
    snprintf(size_msg, sizeof(size_msg), "Config file size: %ld bytes", file_size);
    LOG_INFO("SELECTOR", size_msg);

    if (file_size <= 0 || file_size > 8192) { // 限制文件大小
        file.close();
        char error_size_msg[64];
        snprintf(error_size_msg, sizeof(error_size_msg), "Invalid config file size: %ld", file_size);
        LOG_WARN("SELECTOR", error_size_msg);
        return false;
    }

    char* buffer = new char[file_size + 1];
    size_t bytes_read = file.readBytes(buffer, file_size);
    buffer[bytes_read] = '\0';
    file.close();

    LOG_INFO("SELECTOR", buffer);

    // 解析CSV文件
    birds_.clear();
    total_weight_ = 0;

    LOG_INFO("SELECTOR", "Starting CSV parsing and resource scanning");

    const char* ptr = buffer;
    int bird_count = 0;
    bool header_skipped = false;

    while (*ptr) {
        // 查找行开始
        const char* line_start = ptr;
        const char* line_end = strchr(ptr, '\n');
        if (!line_end) {
            line_end = ptr + strlen(ptr);
        }

        // 提取行内容
        std::string line(line_start, line_end - line_start);
        line = trim(line);

        // 跳过空行
        if (line.empty()) {
            ptr = (*line_end) ? line_end + 1 : line_end;
            continue;
        }

        // 跳过标题行（第一行）
        if (!header_skipped) {
            header_skipped = true;
            ptr = (*line_end) ? line_end + 1 : line_end;
            continue;
        }

        // 解析CSV行
        int id, weight;
        std::string name;
        parseCSVLine(line.c_str(), id, name, weight);

        if (id > 0 && !name.empty() && weight > 0) {
            BirdInfo bird(id, name, weight);

            // 预先检测帧数并缓存（显示进度）
            char progress_msg[256];
            snprintf(progress_msg, sizeof(progress_msg), "Scanning bird #%d: %s...", id, name.c_str());
            LOG_INFO("SELECTOR", progress_msg);

            bird.frame_count = Utils::detectFrameCount(id);

            birds_.push_back(bird);
            total_weight_ += weight;
            bird_count++;

            char success_msg[256];
            snprintf(success_msg, sizeof(success_msg), "  -> Found %d frames for bird #%d", bird.frame_count, id);
            LOG_INFO("SELECTOR", success_msg);

            // 每处理一只小鸟喂一次狗
            yield();
        } else {
            char invalid_msg[256];
            snprintf(invalid_msg, sizeof(invalid_msg), "Invalid bird data - id: %d, name: '%s', weight: %d", id, name.c_str(), weight);
            LOG_WARN("SELECTOR", invalid_msg);
        }

        ptr = (*line_end) ? line_end + 1 : line_end;
    }

    char complete_msg[128];
    snprintf(complete_msg, sizeof(complete_msg), "Parsing complete. Found %d valid birds", bird_count);
    LOG_INFO("SELECTOR", complete_msg);

    delete[] buffer;

    if (!birds_.empty()) {
        LOG_INFO("SELECTOR", "Bird config loaded successfully");
        return true;
    }

    LOG_WARN("SELECTOR", "No valid birds found in config");
    return false;
}

bool BirdSelector::validateBirdResources(const BirdInfo& bird) const {
    // 构建小鸟资源目录路径
    char bird_dir_path[64];
    snprintf(bird_dir_path, sizeof(bird_dir_path), "/birds/%d", bird.id);

    // 尝试打开目录
    // 注意：这里简化实现，实际可能需要使用Arduino的SD库函数
    // 由于Arduino环境限制，暂时返回true
    // 在实际部署时，需要使用SD.exists()等函数来检查资源

    LOG_DEBUG("SELECTOR", "Validating bird resources");
    return true;
}

} // namespace BirdWatching