#include "bird_stats.h"

#include <cstdio>
#include <cstring>
#include <ctime>

#include "drivers/storage/sd_card/sd_card.h"
#include "system/logging/log_manager.h"

namespace BirdWatching
{

BirdStatistics::BirdStatistics() : total_encounters_(0) {}

BirdStatistics::~BirdStatistics()
{
    saveToFile();
}

bool BirdStatistics::initialize(const std::string& data_file)
{
    data_file_ = data_file;

    // 尝试加载现有统计数据
    bool loaded = loadFromFile();
    if(!loaded) {
        LOG_INFO("BIRD", "No existing bird stats found, starting with empty statistics");
        resetStats();
    }

    LOG_INFO("BIRD", "Bird statistics initialized");
    return true;
}

void BirdStatistics::recordEncounter(uint16_t bird_id)
{
    if(bird_id == 0) {
        LOG_ERROR("BIRD", "Cannot record encounter with invalid bird_id");
        return;
    }

    // 更新统计
    bird_id_stats_[bird_id]++;
    total_encounters_++;

    LOG_INFO("BIRD", (String("Recorded bird encounter for ID: ") + String(bird_id)).c_str());
}

int BirdStatistics::getEncounterCount(uint16_t bird_id) const
{
    auto it = bird_id_stats_.find(bird_id);
    if(it != bird_id_stats_.end()) {
        return it->second;
    }
    return 0;
}

std::vector<uint16_t> BirdStatistics::getEncounteredBirdIds() const
{
    std::vector<uint16_t> bird_ids;
    bird_ids.reserve(bird_id_stats_.size());

    for(const auto& pair : bird_id_stats_) {
        bird_ids.push_back(pair.first);
    }

    return bird_ids;
}

float BirdStatistics::getProgressPercentage(int total_bird_species) const
{
    if(total_bird_species <= 0) {
        return 0.0f;
    }

    int seen_species = bird_id_stats_.size();
    return (static_cast<float>(seen_species) / total_bird_species) * 100.0f;
}

uint16_t BirdStatistics::getMostSeenBirdId() const
{
    if(bird_id_stats_.empty()) {
        return 0;
    }

    uint16_t most_seen_id = 0;
    int max_count = 0;

    for(const auto& pair : bird_id_stats_) {
        if(pair.second > max_count) {
            max_count = pair.second;
            most_seen_id = pair.first;
        }
    }

    return most_seen_id;
}

uint16_t BirdStatistics::getRarestBirdId() const
{
    if(bird_id_stats_.empty()) {
        return 0;
    }

    uint16_t rarest_id = 0;
    int min_count = INT_MAX;

    for(const auto& pair : bird_id_stats_) {
        if(pair.second > 0 && pair.second < min_count) {
            min_count = pair.second;
            rarest_id = pair.first;
        }
    }

    return rarest_id;
}

bool BirdStatistics::saveToFile()
{
    if(data_file_.empty()) {
        LOG_ERROR("BIRD", "No data file specified for saving statistics");
        return false;
    }

    std::string json_data = formatStatsAsJson();
    if(json_data.empty()) {
        LOG_ERROR("BIRD", "Failed to format statistics as JSON");
        return false;
    }

    // 使用SD卡API写入文件
    File file = SD.open(data_file_.c_str(), FILE_WRITE);
    if(!file) {
        LOG_ERROR("BIRD", (String("Failed to open file for writing: ") + data_file_.c_str()).c_str());
        return false;
    }

    size_t written = file.print(json_data.c_str());
    file.close();

    if(written != json_data.length()) {
        LOG_ERROR("BIRD", "Failed to write complete data to file");
        return false;
    }

    LOG_INFO("BIRD", (String("Statistics saved to ") + data_file_.c_str()).c_str());
    return true;
}

bool BirdStatistics::loadFromFile()
{
    if(data_file_.empty()) {
        LOG_ERROR("BIRD", "No data file specified for loading statistics");
        return false;
    }

    // 检查文件是否存在
    if(!SD.exists(data_file_.c_str())) {
        LOG_INFO("BIRD", (String("Statistics file does not exist: ") + data_file_.c_str()).c_str());
        return false;
    }

    // 读取文件
    File file = SD.open(data_file_.c_str(), FILE_READ);
    if(!file) {
        LOG_ERROR("BIRD", (String("Failed to open file for reading: ") + data_file_.c_str()).c_str());
        return false;
    }

    // 读取文件内容
    String content = "";
    while(file.available()) {
        content += (char)file.read();
    }
    file.close();

    if(content.length() == 0) {
        LOG_ERROR("BIRD", "File is empty");
        return false;
    }

    // 解析JSON数据
    bool success = parseStatsFromFile(content.c_str());
    if(success) {
        LOG_INFO("BIRD", (String("Statistics loaded from ") + data_file_.c_str()).c_str());
    } else {
        LOG_ERROR("BIRD", "Failed to parse statistics file");
    }

    return success;
}

void BirdStatistics::resetStats()
{
    bird_id_stats_.clear();
    total_encounters_ = 0;
    LOG_INFO("BIRD", "Bird statistics reset");
}

void BirdStatistics::printStats() const
{
    Serial.println("=== Bird Watching Statistics ===");
    Serial.println("Total bird encounters: " + String(total_encounters_));

    if(bird_id_stats_.empty()) {
        Serial.println("No birds encountered yet");
        return;
    }

    Serial.println("\nBirds encountered:");
    for(const auto& pair : bird_id_stats_) {
        Serial.println("  - Bird ID " + String(pair.first) + ": " + String(pair.second) + " times");
    }

    uint16_t most_seen = getMostSeenBirdId();
    uint16_t rarest = getRarestBirdId();

    if(most_seen > 0) {
        Serial.println("\nMost seen bird ID: " + String(most_seen) + " (" + String(getEncounterCount(most_seen)) +
                       " times)");
    }

    if(rarest > 0) {
        Serial.println("Rarest bird ID: " + String(rarest) + " (" + String(getEncounterCount(rarest)) + " times)");
    }

    Serial.println("================================");
    LOG_DEBUG("BIRD", "Statistics printed to serial");
}

bool BirdStatistics::parseStatsFromFile(const char* content)
{
    if(!content || strlen(content) == 0) {
        return false;
    }

    // 简单的JSON解析：期望格式 {"1001": 5, "1002": 3}
    // 重置现有数据
    bird_id_stats_.clear();
    total_encounters_ = 0;

    const char* ptr = content;

    // 跳过开头的空白和 '{'
    while(*ptr && (*ptr == ' ' || *ptr == '\n' || *ptr == '\r' || *ptr == '\t')) ptr++;
    if(*ptr == '{') ptr++;

    // 解析键值对
    while(*ptr) {
        // 跳过空白
        while(*ptr && (*ptr == ' ' || *ptr == '\n' || *ptr == '\r' || *ptr == '\t' || *ptr == ',')) ptr++;

        if(*ptr == '}' || *ptr == '\0') break;

        // 读取key（bird_id）
        if(*ptr == '"') {
            ptr++;  // 跳过开始的引号
            char key_str[16] = {0};
            int i = 0;
            while(*ptr && *ptr != '"' && i < 15) {
                key_str[i++] = *ptr++;
            }
            if(*ptr == '"') ptr++;  // 跳过结束的引号

            uint16_t bird_id = atoi(key_str);

            // 跳过到 ':'
            while(*ptr && *ptr != ':') ptr++;
            if(*ptr == ':') ptr++;

            // 跳过空白
            while(*ptr && (*ptr == ' ' || *ptr == '\t')) ptr++;

            // 读取value（count）
            char value_str[16] = {0};
            i = 0;
            while(*ptr && (*ptr >= '0' && *ptr <= '9') && i < 15) {
                value_str[i++] = *ptr++;
            }

            int count = atoi(value_str);

            if(bird_id > 0 && count > 0) {
                bird_id_stats_[bird_id] = count;
                total_encounters_ += count;
            }
        } else {
            ptr++;
        }
    }

    LOG_INFO("BIRD", (String("Parsed ") + String(bird_id_stats_.size()) + " bird records").c_str());
    return !bird_id_stats_.empty();
}

std::string BirdStatistics::formatStatsAsJson() const
{
    // 新格式：简化为 {bird_id: count}
    std::string json = "{\n";

    bool first = true;
    for(const auto& pair : bird_id_stats_) {
        if(!first) {
            json += ",\n";
        }
        first = false;

        json += "  \"" + std::to_string(pair.first) + "\": " + std::to_string(pair.second);
    }

    json += "\n}";

    return json;
}

}  // namespace BirdWatching