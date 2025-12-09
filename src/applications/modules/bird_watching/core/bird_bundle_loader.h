#ifndef BIRD_BUNDLE_LOADER_H
#define BIRD_BUNDLE_LOADER_H

#include <Arduino.h>
#include <SD.h>
#include <lvgl.h>
#include <string>
#include <vector>

namespace BirdWatching {

/**
 * Bundle文件头部结构 (64字节)
 *
 * 与Python端rgb565.py中的格式保持一致
 */
struct BirdBundleHeader {
    uint32_t magic;          // 0x42495244 ("BIRD")
    uint16_t version;        // 版本号 (当前: 1)
    uint16_t frame_count;    // 总帧数
    uint16_t frame_width;    // 帧宽度 (120)
    uint16_t frame_height;   // 帧高度 (120)
    uint32_t frame_size;     // 单帧大小（字节，含LVGL头）
    uint32_t index_offset;   // 索引表偏移量（通常为64）
    uint32_t data_offset;    // 数据区偏移量
    uint32_t total_size;     // 文件总大小
    uint8_t  color_format;   // 颜色格式 (0x12=RGB565)
    uint8_t  reserved[35];   // 保留字段
} __attribute__((packed));

/**
 * 帧索引条目 (12字节)
 */
struct FrameIndexEntry {
    uint32_t offset;         // 帧数据偏移量（从文件开头）
    uint32_t size;           // 帧数据大小（字节）
    uint32_t checksum;       // CRC32校验（可选）
} __attribute__((packed));

/**
 * Bundle文件加载器
 *
 * 用于从单个bundle.bin文件中按需加载帧数据，减少SD卡文件打开次数
 */
class BirdBundleLoader {
public:
    BirdBundleLoader();
    ~BirdBundleLoader();

    /**
     * 加载bundle文件
     *
     * @param bundle_path Bundle文件路径 (例如: "/birds/1001/bundle.bin")
     * @return 成功返回true
     */
    bool loadBundle(const std::string& bundle_path);

    /**
     * 从bundle中加载指定帧
     *
     * @param frame_index 帧索引 (0-based，最大65535)
     * @param out_dsc 输出LVGL图像描述符指针
     * @param out_data 输出图像数据指针
     * @return 成功返回true
     */
    bool loadFrame(uint16_t frame_index, lv_image_dsc_t** out_dsc, uint8_t** out_data);

    /**
     * 获取bundle中的帧数
     */
    uint16_t getFrameCount() const { return header_.frame_count; }

    /**
     * 获取帧宽度
     */
    uint16_t getFrameWidth() const { return header_.frame_width; }

    /**
     * 获取帧高度
     */
    uint16_t getFrameHeight() const { return header_.frame_height; }

    /**
     * 检查bundle是否已加载
     */
    bool isLoaded() const { return is_loaded_; }

    /**
     * 关闭bundle
     */
    void close();

private:
    BirdBundleHeader header_;
    std::vector<FrameIndexEntry> index_table_;
    std::string bundle_path_;
    bool is_loaded_;

    /**
     * 验证bundle文件头部
     */
    bool validateHeader();
};

} // namespace BirdWatching

#endif // BIRD_BUNDLE_LOADER_H