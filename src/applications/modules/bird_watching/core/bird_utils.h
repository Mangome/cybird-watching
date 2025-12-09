#ifndef BIRD_UTILS_H
#define BIRD_UTILS_H

#include <cstdint>

namespace BirdWatching {
namespace Utils {

/**
 * @brief 从bundle文件检测小鸟的帧数
 *
 * 直接从bundle.bin文件头读取帧数，O(1)时间复杂度
 *
 * @param bird_id 小鸟ID
 * @return 检测到的帧数（最大65535），如果没有找到则返回0
 */
uint16_t detectFrameCount(uint16_t bird_id);

} // namespace Utils
} // namespace BirdWatching

#endif // BIRD_UTILS_H
