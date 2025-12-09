#include "bird_bundle_loader.h"
#include "system/logging/log_manager.h"

namespace BirdWatching {

// Bundle文件魔数: "BIRD"
constexpr uint32_t BUNDLE_MAGIC = 0x42495244;
constexpr uint16_t BUNDLE_VERSION = 1;
constexpr uint8_t RGB565_COLOR_FORMAT = 0x12;

BirdBundleLoader::BirdBundleLoader()
    : is_loaded_(false)
{
}

BirdBundleLoader::~BirdBundleLoader() {
    close();
}

bool BirdBundleLoader::loadBundle(const std::string& bundle_path) {
    // 关闭之前的bundle
    close();

    bundle_path_ = bundle_path;

    // 打开bundle文件
    File file = SD.open(bundle_path.c_str());
    if (!file) {
        LOG_ERROR("BUNDLE", "Failed to open bundle: " + String(bundle_path.c_str()));
        return false;
    }

    // 读取Bundle Header (64字节)
    size_t bytes_read = file.read((uint8_t*)&header_, sizeof(BirdBundleHeader));
    if (bytes_read != sizeof(BirdBundleHeader)) {
        LOG_ERROR("BUNDLE", "Failed to read bundle header");
        file.close();
        return false;
    }

    // 验证header
    if (!validateHeader()) {
        file.close();
        return false;
    }

    // 读取Frame Index表
    index_table_.resize(header_.frame_count);
    file.seek(header_.index_offset);

    size_t index_size = header_.frame_count * sizeof(FrameIndexEntry);
    bytes_read = file.read((uint8_t*)index_table_.data(), index_size);

    if (bytes_read != index_size) {
        LOG_ERROR("BUNDLE", "Failed to read frame index table");
        file.close();
        return false;
    }

    file.close();

    is_loaded_ = true;
    LOG_INFO("BUNDLE", "Bundle loaded: " + String(header_.frame_count) + " frames, " +
             String(header_.frame_width) + "x" + String(header_.frame_height));

    return true;
}

bool BirdBundleLoader::loadFrame(uint16_t frame_index, lv_image_dsc_t** out_dsc, uint8_t** out_data) {
    if (!is_loaded_) {
        LOG_ERROR("BUNDLE", "Bundle not loaded");
        return false;
    }

    if (frame_index >= header_.frame_count) {
        LOG_ERROR("BUNDLE", "Frame index out of range: " + String(frame_index) +
                  "/" + String(header_.frame_count));
        return false;
    }

    if (!out_dsc || !out_data) {
        LOG_ERROR("BUNDLE", "Invalid output parameters");
        return false;
    }

    // 获取帧索引信息
    const FrameIndexEntry& entry = index_table_[frame_index];

    // 打开bundle文件
    File file = SD.open(bundle_path_.c_str());
    if (!file) {
        LOG_ERROR("BUNDLE", "Failed to open bundle for frame reading");
        return false;
    }

    // 定位到帧数据位置
    file.seek(entry.offset);

    // 读取LVGL 9.x头部 (32字节)
    uint32_t header_cf, flags, stride, reserved_2, data_size;
    uint16_t width, height;

    if (file.read((uint8_t*)&header_cf, 4) != 4 ||
        file.read((uint8_t*)&flags, 4) != 4 ||
        file.read((uint8_t*)&width, 2) != 2 ||
        file.read((uint8_t*)&height, 2) != 2 ||
        file.read((uint8_t*)&stride, 4) != 4 ||
        file.read((uint8_t*)&reserved_2, 4) != 4 ||
        file.read((uint8_t*)&data_size, 4) != 4) {
        LOG_ERROR("BUNDLE", "Failed to read LVGL header for frame " + String(frame_index));
        file.close();
        return false;
    }

    // 验证LVGL格式
    uint8_t color_format = header_cf & 0xFF;
    uint8_t magic = (header_cf >> 24) & 0xFF;

    if (color_format != RGB565_COLOR_FORMAT || magic != 0x37) {
        LOG_ERROR("BUNDLE", "Invalid LVGL format in frame " + String(frame_index) +
                  ": cf=0x" + String(color_format, HEX) + ", magic=0x" + String(magic, HEX));
        file.close();
        return false;
    }

    // 检查可用内存
    size_t free_heap = ESP.getFreeHeap();
    if (free_heap < data_size + 4096) {
        LOG_ERROR("BUNDLE", "Insufficient memory - need " + String(data_size) +
                  " + 4096, have " + String(free_heap));
        file.close();
        return false;
    }

    // 分配内存
    lv_image_dsc_t* img_dsc = static_cast<lv_image_dsc_t*>(malloc(sizeof(lv_image_dsc_t)));
    uint8_t* img_data = static_cast<uint8_t*>(malloc(data_size));

    if (!img_dsc || !img_data) {
        LOG_ERROR("BUNDLE", "Failed to allocate memory for frame " + String(frame_index));
        if (img_dsc) free(img_dsc);
        if (img_data) free(img_data);
        file.close();
        return false;
    }

    // 读取像素数据
    size_t bytes_read = file.read(img_data, data_size);
    file.close();

    // 让出CPU，避免看门狗超时
    vTaskDelay(1);

    if (bytes_read != data_size) {
        LOG_ERROR("BUNDLE", "Failed to read pixel data: " + String(bytes_read) +
                  "/" + String(data_size));
        free(img_dsc);
        free(img_data);
        return false;
    }

    // 设置LVGL图像描述符 - LVGL 9.x格式
    img_dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
    img_dsc->header.cf = color_format;
    img_dsc->header.flags = 0;
    img_dsc->header.w = width;
    img_dsc->header.h = height;
    img_dsc->header.stride = width * 2;  // RGB565每像素2字节
    img_dsc->header.reserved_2 = 0;
    img_dsc->data_size = data_size;
    img_dsc->data = img_data;

    *out_dsc = img_dsc;
    *out_data = img_data;

    return true;
}

void BirdBundleLoader::close() {
    if (is_loaded_) {
        index_table_.clear();
        bundle_path_.clear();
        is_loaded_ = false;
    }
}

bool BirdBundleLoader::validateHeader() {
    // 验证魔数
    if (header_.magic != BUNDLE_MAGIC) {
        LOG_ERROR("BUNDLE", "Invalid magic number: 0x" + String(header_.magic, HEX) +
                  " (expected 0x" + String(BUNDLE_MAGIC, HEX) + ")");
        return false;
    }

    // 验证版本
    if (header_.version != BUNDLE_VERSION) {
        LOG_WARN("BUNDLE", "Bundle version mismatch: " + String(header_.version) +
                 " (expected " + String(BUNDLE_VERSION) + ")");
        // 版本不匹配只是警告，不阻止加载
    }

    // 验证颜色格式
    if (header_.color_format != RGB565_COLOR_FORMAT) {
        LOG_ERROR("BUNDLE", "Unsupported color format: 0x" + String(header_.color_format, HEX));
        return false;
    }

    // 验证帧数
    if (header_.frame_count == 0) {
        LOG_ERROR("BUNDLE", "Invalid frame count: 0");
        return false;
    }

    // 验证尺寸
    if (header_.frame_width == 0 || header_.frame_height == 0) {
        LOG_ERROR("BUNDLE", "Invalid frame dimensions: " + String(header_.frame_width) +
                  "x" + String(header_.frame_height));
        return false;
    }

    return true;
}

} // namespace BirdWatching