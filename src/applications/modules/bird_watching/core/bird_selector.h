#ifndef BIRD_SELECTOR_H
#define BIRD_SELECTOR_H

#include <string>
#include <vector>

#include "bird_types.h"

namespace BirdWatching
{

class BirdSelector
{
public:
    BirdSelector();
    ~BirdSelector();

    // 初始化选择器，加载小鸟列表
    bool initialize(const std::string& config_path = "S:/configs/bird_config.csv");

    // 根据权重随机选择一只小鸟
    BirdInfo getRandomBird();

    // 获取所有可用小鸟
    const std::vector<BirdInfo>& getAllBirds() const
    {
        return birds_;
    }

    // 获取小鸟总数
    size_t getBirdCount() const
    {
        return birds_.size();
    }

    // 根据名称查找小鸟
    const BirdInfo* findBird(const std::string& name) const;

    // 获取总权重
    int getTotalWeight() const
    {
        return total_weight_;
    }

    // 重新加载配置
    bool reloadConfig();

private:
    std::vector<BirdInfo> birds_;  // 小鸟列表
    int total_weight_;             // 总权重

    // 从JSON配置文件加载小鸟列表
    bool loadBirdConfig(const std::string& config_path);

    // 从单个小鸟目录加载信息
    bool loadBirdInfo(const std::string& bird_dir_path);

    // 验证小鸟资源是否完整
    bool validateBirdResources(const BirdInfo& bird) const;
};

}  // namespace BirdWatching

#endif  // BIRD_SELECTOR_H