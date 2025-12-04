#include "bird_utils.h"
#include "drivers/storage/sd_card/sd_card.h"
#include <cstdio>
#include <Arduino.h>

namespace BirdWatching {
namespace Utils {

uint8_t detectFrameCount(uint16_t bird_id) {
    // 优化策略：结合常见帧数快速路径和二分查找
    uint8_t max_frames = 0;
    uint8_t lower_bound = 1;  // 记录下界
    
    // 首先尝试常见的帧数（从大到小，快速路径）
    // 80是当前所有动画的标准帧数，优先检查
    const uint8_t common_counts[] = {80, 64, 48, 32, 24, 16, 8};
    for (uint8_t test_count : common_counts) {
        char path[128];
        snprintf(path, sizeof(path), "/birds/%d/%d.bin", bird_id, test_count);
        File test_file = SD.open(path);
        if (test_file) {
            test_file.close();
            max_frames = test_count;
            // 找到了魔数，立即检查下一帧是否存在
            snprintf(path, sizeof(path), "/birds/%d/%d.bin", bird_id, test_count + 1);
            File next_file = SD.open(path);
            if (!next_file) {
                // 下一帧不存在，魔数就是最大帧数
                return max_frames;
            }
            next_file.close();
            // 下一帧存在，说明帧数 > test_count，继续向上查找
            lower_bound = test_count + 2;  // 已经确认了 test_count+1 存在
            max_frames = test_count + 1;
            break;
        }
        // 没找到，继续尝试更小的常见值
    }
    
    // 如果找到了一个常见值但还有更多帧，使用二分查找在 [lower_bound, 100] 范围内精确定位
    if (max_frames > 0 && lower_bound <= 100) {
        uint8_t min_val = lower_bound;
        uint8_t max_val = 100;
        
        // 二分查找找到确切的最大帧数
        while (min_val <= max_val) {
            uint8_t mid = (min_val + max_val) / 2;
            char path[128];
            snprintf(path, sizeof(path), "/birds/%d/%d.bin", bird_id, mid);
            File test_file = SD.open(path);
            
            if (test_file) {
                test_file.close();
                max_frames = mid;  // 找到更大的帧数
                min_val = mid + 1; // 继续向上查找
            } else {
                max_val = mid - 1; // 向下查找
            }
        }
    } else if (max_frames == 0) {
        // 没找到任何常见值，说明帧数 < 8，线性查找
        for (uint8_t i = 1; i < 8; i++) {
            char path[128];
            snprintf(path, sizeof(path), "/birds/%d/%d.bin", bird_id, i);
            File test_file = SD.open(path);
            if (test_file) {
                test_file.close();
                max_frames = i;
            } else if (max_frames > 0) {
                break; // 找到缺口，停止查找
            }
        }
    }
    
    return max_frames;
}

} // namespace Utils
} // namespace BirdWatching
