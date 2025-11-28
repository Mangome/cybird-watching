#include "bird_animation.h"
#include "system/logging/log_manager.h"
#include "drivers/storage/sd_card/sd_card.h"
#include <cstring>
#include <cstdio>

namespace BirdWatching {

BirdAnimation::BirdAnimation()
    : display_obj_(nullptr)
    , current_frame_(0)
    , current_frame_count_(0)
    , play_timer_(nullptr)
    , is_playing_(false)
    , frame_duration_(125) // 默认8fps = 125ms每帧
    , current_img_dsc_(nullptr)
    , current_img_data_(nullptr)
{
}

BirdAnimation::~BirdAnimation() {
    stop();
    releasePreviousFrame();
}

bool BirdAnimation::init(lv_obj_t* parent_obj) {
    if (!parent_obj) {
        // 如果没有提供父对象，使用当前活动屏幕
        parent_obj = lv_scr_act();
    }

    // 创建或设置显示对象
    if (!display_obj_) {
        display_obj_ = lv_img_create(parent_obj, nullptr);
        if (!display_obj_) {
            LOG_ERROR("ANIM", "Failed to create LVGL image object");
            return false;
        }

        // 设置对象大小和位置
        lv_obj_set_size(display_obj_, 240, 240);
        lv_obj_set_pos(display_obj_, 0, 0);
    }

    LOG_INFO("ANIM", "Bird animation system initialized");
    return true;
}

bool BirdAnimation::loadBird(const BirdInfo& bird_info) {
    // 停止当前动画
    stop();

    // 设置小鸟信息
    current_bird_ = bird_info;
    current_frame_ = 0;

    // 自动检测帧数
    current_frame_count_ = detectFrameCount(current_bird_.id);
    if (current_frame_count_ == 0) {
        LOG_WARN("ANIM", "No frames found for bird, using default");
        current_frame_count_ = 8; // 默认帧数
    }

    // 使用全局固定的帧率（8fps，可配置）
    frame_duration_ = 125; // 1000ms / 8fps = 125ms，后续可配置

    LOG_INFO("ANIM", "Bird loaded successfully");
    LOG_DEBUG("ANIM", "Bird animation details loaded");
    return true;
}

void BirdAnimation::startLoop() {
    if (is_playing_) {
        LOG_WARN("ANIM", "Animation already playing, stopping previous animation");
        stop();
    }

    if (current_bird_.id == 0) {
        LOG_ERROR("ANIM", "No bird loaded for animation");
        return;
    }

    // 重置到第一帧
    current_frame_ = 0;

    // 加载并显示第一帧
    if (!loadAndShowFrame(0)) {
        LOG_ERROR("ANIM", "Failed to load first frame for bird");
        return;
    }

    // 创建播放定时器
    play_timer_ = lv_task_create(timerCallback, frame_duration_, LV_TASK_PRIO_MID, this);
    if (!play_timer_) {
        LOG_ERROR("ANIM", "Failed to create animation timer");
        return;
    }

    is_playing_ = true;
    LOG_INFO("ANIM", "Started loop animation for bird");
}

void BirdAnimation::stop() {
    if (play_timer_) {
        lv_task_del(play_timer_);
        play_timer_ = nullptr;
    }
    is_playing_ = false;
    current_frame_ = 0;

    // 释放图像内存
    releasePreviousFrame();

    // 清除显示内容
    if (display_obj_) {
        lv_img_set_src(display_obj_, nullptr);
    }

    LOG_INFO("ANIM", "Animation stopped");
}

void BirdAnimation::setDisplayObject(lv_obj_t* obj) {
    if (is_playing_) {
        stop();
    }
    display_obj_ = obj;
}

std::string BirdAnimation::getFramePath(uint8_t frame_index) const {
    char path[128];
    snprintf(path, sizeof(path), "/birds/%d/pic-%02d.bin",
             current_bird_.id, frame_index + 1); // 从1开始编号，格式为pic-01.bin, pic-02.bin等
    return std::string(path);
}

bool BirdAnimation::loadAndShowFrame(uint8_t frame_index) {
    if (!display_obj_) {
        LOG_ERROR("BIRD", "Display object not set");
        return false;
    }

    if (frame_index >= current_frame_count_) {
        LOG_ERROR("BIRD", "Frame index out of range");
        return false;
    }

    std::string frame_path = getFramePath(frame_index);

    // 先尝试手动加载，因为LVGL 7.9.1对.bin文件支持有问题
    // 静默处理以避免日志洪水

    // 尝试手动加载图像
    if (tryManualImageLoad(frame_path)) {
        // 成功时不输出日志避免洪水
    } else {
        // 手动加载失败，使用后备颜色显示
        lv_color_t bird_color = lv_color_hex(0x808080); // 默认灰色
        switch (current_bird_.id % 8) {
            case 1: bird_color = lv_color_hex(0x8B4513); break;
            case 2: bird_color = lv_color_hex(0xB22222); break;
            case 3: bird_color = lv_color_hex(0x4682B4); break;
            case 4: bird_color = lv_color_hex(0x00008B); break;
            case 5: bird_color = lv_color_hex(0x228B22); break;
            case 6: bird_color = lv_color_hex(0xFFD700); break;
            case 7: bird_color = lv_color_hex(0xFF69B4); break;
        }

        lv_obj_set_style_local_bg_color(display_obj_, LV_OBJ_PART_MAIN, 0, bird_color);
        lv_obj_set_style_local_border_width(display_obj_, LV_OBJ_PART_MAIN, 0, 2);
        lv_obj_set_style_local_border_color(display_obj_, LV_OBJ_PART_MAIN, 0, lv_color_hex(0x333333));
    }

    return true;
}

void BirdAnimation::playNextFrame() {
    if (!is_playing_) {
        return;
    }

    current_frame_++;

    // 循环播放：当到达最后一帧时回到第一帧
    if (current_frame_ >= current_frame_count_) {
        current_frame_ = 0;  // 回到第一帧继续循环
        LOG_DEBUG("ANIM", "Animation loop, restarting from first frame");
    }

    // 加载并显示下一帧
    if (!loadAndShowFrame(current_frame_)) {
        LOG_ERROR("ANIM", "Failed to load frame");
        stop();
        return;
    }

    LOG_DEBUG("ANIM", "Playing next frame in loop");
}

uint8_t BirdAnimation::detectFrameCount(uint16_t bird_id) const {
    uint8_t count = 0;
    char frame_path[64];

    // 从1开始递增检测，直到文件不存在
    for (uint8_t i = 1; ; i++) {
        snprintf(frame_path, sizeof(frame_path), "/birds/%d/pic-%02d.bin", bird_id, i);

        // 使用SD.exists检测文件是否存在
        if (SD.exists(frame_path)) {
            count++;
        } else {
            break; // 文件不存在，停止检测
        }
    }

    LOG_DEBUG("ANIM", "Detected frame count");
    return count; // 返回实际检测到的帧数
}

bool BirdAnimation::tryManualImageLoad(const std::string& file_path) {
    // 静默处理避免日志洪水

    // 使用项目的SD卡接口
    File file = SD.open(file_path.c_str());
    if (!file) {
        return false;
    }

    size_t file_size = file.size();
    if (file_size < 12) { // 最小LVGL头部大小
        LOG_ERROR("BIRD", "File too small: " + String(file_size) + " bytes");
        file.close();
        return false;
    }

    // 读取LVGL头部
    uint32_t cf, data_size;
    uint16_t width, height;

    if (file.read((uint8_t*)&cf, sizeof(cf)) != sizeof(cf) ||
        file.read((uint8_t*)&width, sizeof(width)) != sizeof(width) ||
        file.read((uint8_t*)&height, sizeof(height)) != sizeof(height) ||
        file.read((uint8_t*)&data_size, sizeof(data_size)) != sizeof(data_size)) {
        LOG_ERROR("BIRD", "Failed to read header");
        file.close();
        return false;
    }

    // 验证文件格式
    uint8_t color_format = cf & 0xFF;

    if (color_format != 4) { // 期望RGB565格式
        file.close();
        return false;
    }

    // 验证数据大小
    if (data_size != width * height * 2) {
        file.close();
        return false;
    }

    // 检查可用内存
    size_t free_heap = ESP.getFreeHeap();

    if (free_heap < data_size + 4096) { // 预留4KB额外空间
        file.close();
        return false;
    }

    // 分配内存
    lv_img_dsc_t* img_dsc = static_cast<lv_img_dsc_t*>(malloc(sizeof(lv_img_dsc_t)));
    if (!img_dsc) {
        LOG_ERROR("BIRD", "Failed to allocate descriptor");
        file.close();
        return false;
    }

    uint8_t* img_data = static_cast<uint8_t*>(malloc(data_size));
    if (!img_data) {
        LOG_ERROR("BIRD", "Failed to allocate image data");
        free(img_dsc);
        file.close();
        return false;
    }

    // 读取像素数据
    size_t bytes_read = file.read(img_data, data_size);
    file.close();

    if (bytes_read != data_size) {
        LOG_ERROR("BIRD", "Failed to read pixel data: " + String(bytes_read) + "/" + String(data_size));
        free(img_dsc);
        free(img_data);
        return false;
    }

    // 设置LVGL图像描述符 - LVGL 7.x格式
    img_dsc->header.cf = color_format;  // 应该是4 (RGB565)
    img_dsc->header.w = width;
    img_dsc->header.h = height;
    img_dsc->header.always_zero = 0;
    img_dsc->header.reserved = 0;
    img_dsc->data_size = data_size;
    img_dsc->data = img_data;

    
    // 释放前一帧的内存
    releasePreviousFrame();

    // 保存当前帧的引用
    current_img_dsc_ = img_dsc;
    current_img_data_ = img_data;

    // 设置图像源
    lv_img_set_src(display_obj_, img_dsc);

    // 调整显示对象大小以匹配图像，或居中显示
    if (width != 240 || height != 240) {
        // 计算居中位置
        int16_t x = (240 - width) / 2;
        int16_t y = (240 - height) / 2;
        lv_obj_set_pos(display_obj_, x, y);
        lv_obj_set_size(display_obj_, width, height);
    }

    // 确保对象可见 - LVGL 7.x兼容
    lv_obj_set_hidden(display_obj_, false);

    // 测试已完成，现在显示真实图像
    // if (false) { // 改为false来显示真实图像
    //     createTestImage();
    //     return true;
    // }

    return true;
}

void BirdAnimation::releasePreviousFrame() {
    // 释放前一帧的内存
    if (current_img_data_) {
        free(current_img_data_);
        current_img_data_ = nullptr;
    }

    if (current_img_dsc_) {
        free(current_img_dsc_);
        current_img_dsc_ = nullptr;
    }
}

void BirdAnimation::createTestImage() {
    // 创建一个60x60的红色测试图像
    const int width = 60;
    const int height = 60;
    const size_t data_size = width * height * 2; // RGB565

    // 释放前一帧
    releasePreviousFrame();

    // 分配内存
    lv_img_dsc_t* img_dsc = static_cast<lv_img_dsc_t*>(malloc(sizeof(lv_img_dsc_t)));
    uint8_t* img_data = static_cast<uint8_t*>(malloc(data_size));

    if (!img_dsc || !img_data) {
        LOG_ERROR("BIRD", "Failed to allocate test image");
        if (img_dsc) free(img_dsc);
        if (img_data) free(img_data);
        return;
    }

    // 填充红色数据 (RGB565: 红色 = 0xF800)
    uint16_t* pixel_data = (uint16_t*)img_data;
    for (int i = 0; i < width * height; i++) {
        pixel_data[i] = 0xF800; // 纯红色
    }

    // 设置图像描述符
    img_dsc->header.cf = 4; // RGB565
    img_dsc->header.w = width;
    img_dsc->header.h = height;
    img_dsc->header.always_zero = 0;
    img_dsc->header.reserved = 0;
    img_dsc->data_size = data_size;
    img_dsc->data = img_data;

    // 保存引用
    current_img_dsc_ = img_dsc;
    current_img_data_ = img_data;

    // 设置到显示对象
    lv_img_set_src(display_obj_, img_dsc);

    // 居中显示
    int16_t x = (240 - width) / 2;
    int16_t y = (240 - height) / 2;
    lv_obj_set_pos(display_obj_, x, y);
    lv_obj_set_size(display_obj_, width, height);
}

void BirdAnimation::timerCallback(lv_task_t* timer) {
    BirdAnimation* animation = static_cast<BirdAnimation*>(timer->user_data);
    if (animation) {
        animation->playNextFrame();
    }
}

} // namespace BirdWatching