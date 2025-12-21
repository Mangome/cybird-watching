#include "bird_utils.h"
#include "hal/sd_interface.h"
#include <cstdio>
#include <Arduino.h>

namespace BirdWatching {
namespace Utils {

// Bundle文件魔数: "BIRD"
constexpr uint32_t BUNDLE_MAGIC = 0x42495244;
constexpr uint8_t RGB565_COLOR_FORMAT = 0x12;

uint16_t detectFrameCount(uint16_t bird_id) {
    // Bundle模式：直接从bundle文件头读取帧数
    char bundle_path[64];
    snprintf(bundle_path, sizeof(bundle_path), "/birds/%d/bundle.bin", bird_id);

    fs::FS& fs = HAL::SDInterface::getFS();
    File bundle_file = fs.open(bundle_path);
    if (!bundle_file) {
        Serial.printf("[WARN] Bundle not found: %s\n", bundle_path);
        return 0;
    }

    // 读取bundle文件头的前20字节（足够获取frame_count）
    // Bundle Header结构：
    // - magic (4B)
    // - version (2B)
    // - frame_count (2B) <-- 我们需要这个
    // - frame_width (2B)
    // - frame_height (2B)
    // ...

    uint32_t magic = 0;
    uint16_t version = 0;
    uint16_t frame_count = 0;
    uint16_t frame_width = 0;
    uint16_t frame_height = 0;
    uint32_t frame_size = 0;

    // 读取关键字段
    if (bundle_file.read((uint8_t*)&magic, 4) != 4 ||
        bundle_file.read((uint8_t*)&version, 2) != 2 ||
        bundle_file.read((uint8_t*)&frame_count, 2) != 2 ||
        bundle_file.read((uint8_t*)&frame_width, 2) != 2 ||
        bundle_file.read((uint8_t*)&frame_height, 2) != 2 ||
        bundle_file.read((uint8_t*)&frame_size, 4) != 4) {
        Serial.printf("[ERROR] Failed to read bundle header: %s\n", bundle_path);
        bundle_file.close();
        return 0;
    }

    bundle_file.close();

    // 验证魔数
    if (magic != BUNDLE_MAGIC) {
        Serial.printf("[ERROR] Invalid bundle magic: 0x%08X (expected 0x%08X)\n", magic, BUNDLE_MAGIC);
        return 0;
    }

    // 验证帧数合理性（支持最多65535帧）
    if (frame_count == 0 || frame_count > 65535) {
        Serial.printf("[WARN] Suspicious frame count: %d\n", frame_count);
        return 0;
    }

    Serial.printf("[INFO] Bundle detected: %d frames, %dx%d\n", frame_count, frame_width, frame_height);
    return frame_count;
}

} // namespace Utils
} // namespace BirdWatching
