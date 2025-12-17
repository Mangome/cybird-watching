#ifndef BIRD_STATS_H
#define BIRD_STATS_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace BirdWatching
{

class BirdStatistics
{
public:
    BirdStatistics();
    ~BirdStatistics();

    // 初始化统计数据
    bool initialize(const std::string& data_file = "/db.json");

    // 记录一次小鸟遇见（使用bird_id）
    void recordEncounter(uint16_t bird_id);

    // 获取总观鸟次数
    int getTotalEncounters() const
    {
        return total_encounters_;
    }

    // 获取指定小鸟的统计信息（通过ID）
    int getEncounterCount(uint16_t bird_id) const;

    // 获取所有有统计数据的小鸟ID列表
    std::vector<uint16_t> getEncounteredBirdIds() const;

    // 检查是否有历史统计数据
    bool hasHistoricalData() const
    {
        return !bird_id_stats_.empty();
    }

    // 获取观鸟进度百分比（遇到的不同种类占总种类的比例）
    float getProgressPercentage(int total_bird_species) const;

    // 获取最常遇见的小鸟ID
    uint16_t getMostSeenBirdId() const;

    // 获取最稀有的小鸟ID
    uint16_t getRarestBirdId() const;

    // 保存统计数据到文件
    bool saveToFile();

    // 从文件加载统计数据
    bool loadFromFile();

    // 重置统计数据
    void resetStats();

    // 打印统计信息到日志
    void printStats() const;

private:
    std::map<uint16_t, int> bird_id_stats_;  // 统计数据：{bird_id: count}
    int total_encounters_;                   // 总遇见次数
    std::string data_file_;                  // 数据文件路径

    // 从文件解析统计数据
    bool parseStatsFromFile(const char* content);

    // 将统计数据格式化为JSON字符串
    std::string formatStatsAsJson() const;
};

}  // namespace BirdWatching

#endif  // BIRD_STATS_H