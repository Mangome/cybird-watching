#include "bird_animation.h"
#include "system/logging/log_manager.h"
#include "drivers/storage/sd_card/sd_card.h"
#include "system/tasks/task_manager.h"
#include <cstring>
#include <cstdio>

namespace BirdWatching {

BirdAnimation::BirdAnimation()
    : display_obj_(nullptr)
    , current_frame_(0)
    , current_frame_count_(0)
    , play_timer_(nullptr)
    , is_playing_(false)
    , frame_interval_(200)
    , frame_processing_(false)
    , current_img_dsc_(nullptr)
    , current_img_data_(nullptr)
    , running_in_ui_task_(false)
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

    // 检测是否在UI任务中运行
    TaskManager* taskMgr = TaskManager::getInstance();
    if (taskMgr && taskMgr->getUITaskHandle() == xTaskGetCurrentTaskHandle()) {
        running_in_ui_task_ = true;
        LOG_INFO("ANIM", "Running in UI task mode");
    } else {
        running_in_ui_task_ = false;
        LOG_INFO("ANIM", "Running in separate task mode");
    }

    // 创建或设置显示对象
    if (!display_obj_) {
        // 注意: 必须在持有LVGL锁的情况下创建对象
        bool needRelease = false;
        if (!running_in_ui_task_ && taskMgr) {
            if (taskMgr->takeLVGLMutex(1000)) {
                needRelease = true;
            } else {
                LOG_ERROR("ANIM", "Failed to acquire LVGL mutex");
                return false;
            }
        }

        display_obj_ = lv_image_create(parent_obj);
        
        if (needRelease) {
            taskMgr->giveLVGLMutex();
        }

        if (!display_obj_) {
            LOG_ERROR("ANIM", "Failed to create LVGL image object");
            return false;
        }

        // 设置位置（缩放后会自动调整大小）
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

    
    LOG_INFO("ANIM", "Bird loaded successfully");
    LOG_DEBUG("ANIM", "Bird animation details loaded");
    return true;
}

void BirdAnimation::startLoop() {
    if (is_playing_) {
        stop();
    }

    if (current_bird_.id == 0) {
        LOG_ERROR("ANIM", "No bird loaded");
        return;
    }

    // 重置到第一帧
    current_frame_ = 0;
    frame_processing_ = false;
    last_frame_time_ = 0;

    // 加载并显示第一帧
    if (!loadAndShowFrame(0)) {
        LOG_ERROR("ANIM", "Failed to load first frame");
        return;
    }

    // 设置第一帧处理完成的时间，确保第一帧显示足够时间
    last_frame_time_ = millis();

    is_playing_ = true;

    // 设置帧间隔为50ms
    frame_interval_ = 50;

    // 创建播放定时器，50ms后触发下一帧
    play_timer_ = lv_timer_create(timerCallback, 50, this); // LVGL 9.x: 移除优先级参数
    if (!play_timer_) {
        LOG_ERROR("ANIM", "Failed to create animation timer");
        is_playing_ = false;
        return;
    }

    LOG_INFO("ANIM", "Animation timer created successfully");
}

void BirdAnimation::stop() {
    if (play_timer_) {
        lv_timer_del(play_timer_);  // LVGL 9.x: lv_task_del → lv_timer_del
        play_timer_ = nullptr;
    }
    is_playing_ = false;
    frame_processing_ = false;
    current_frame_ = 0;
    last_frame_time_ = 0;

    // 释放图像内存
    releasePreviousFrame();

    // 清除显示内容
    if (display_obj_) {
        lv_image_set_src(display_obj_, nullptr);  // LVGL 9.x: lv_img_set_src → lv_image_set_src
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
    snprintf(path, sizeof(path), "/birds/%d/%d.bin",
             current_bird_.id, frame_index + 1); // 从1开始编号，格式为1.bin, 2.bin等
    return std::string(path);
}

bool BirdAnimation::loadAndShowFrame(uint8_t frame_index) {
    if (!display_obj_) {
        LOG_ERROR("ANIM", "Display object not set");
        return false;
    }

    if (frame_index >= current_frame_count_) {
        LOG_ERROR("ANIM", "Frame index " + String(frame_index) + " out of range");
        return false;
    }

    std::string frame_path = getFramePath(frame_index);

    // 尝试手动加载图像
    if (tryManualImageLoad(frame_path)) {
        // 强制刷新LVGL显示
        lv_obj_invalidate(display_obj_);
        return true;
    } else {
        LOG_WARN("ANIM", "Failed to load frame " + String(frame_index));
        
        // 手动加载失败，使用后备颜色显示
        lv_color_t bird_color = lv_color_hex(0x808080); // 默认灰色
        switch (current_bird_.id % 8) {
            case 0: bird_color = lv_color_hex(0x808080); break; // 灰色
            case 1: bird_color = lv_color_hex(0x8B4513); break; // 棕色
            case 2: bird_color = lv_color_hex(0xB22222); break; // 红色
            case 3: bird_color = lv_color_hex(0x4682B4); break; // 蓝色
            case 4: bird_color = lv_color_hex(0x00008B); break; // 深蓝
            case 5: bird_color = lv_color_hex(0x228B22); break; // 绿色
            case 6: bird_color = lv_color_hex(0xFFD700); break; // 金色
            case 7: bird_color = lv_color_hex(0xFF69B4); break; // 粉色
        }

        lv_obj_set_style_bg_color(display_obj_, bird_color, LV_PART_MAIN);
        lv_obj_set_style_border_width(display_obj_, 2, LV_PART_MAIN);
        lv_obj_set_style_border_color(display_obj_, lv_color_hex(0x333333), LV_PART_MAIN);
        
        // 注意: 返回true以继续动画循环(使用颜色动画)
        return true;
    }
}

void BirdAnimation::playNextFrame() {
    if (!is_playing_ || frame_processing_) {
        return;
    }

    // 标记开始处理帧
    frame_processing_ = true;

    current_frame_++;

    // 循环播放：当到达最后一帧时回到第一帧
    if (current_frame_ >= current_frame_count_) {
        current_frame_ = 0;  // 回到第一帧继续循环
    }

    // 加载并显示下一帧
    if (!loadAndShowFrame(current_frame_)) {
        stop();
        return;
    }

    // 设置下一帧的定时器：50ms后播放下一帧
    frame_processing_ = false;

    // 重新设置定时器，50ms后触发下一帧
    if (play_timer_) {
        lv_timer_del(play_timer_);
    }
    play_timer_ = lv_timer_create(timerCallback, 50, this);
}


uint8_t BirdAnimation::detectFrameCount(uint16_t bird_id) const {
    uint8_t count = 0;
    char frame_path[64];

    // 从1开始递增检测，直到文件不存在
    for (uint8_t i = 1; ; i++) {
        snprintf(frame_path, sizeof(frame_path), "/birds/%d/%d.bin", bird_id, i);

        // 使用SD.exists检测文件是否存在
        if (SD.exists(frame_path)) {
            count++;
        } else {
            break; // 文件不存在，停止检测
        }
    }

    LOG_DEBUG("ANIM", "Detected frame count: " + String(count) + " frames for bird " + String(bird_id));
    return count; // 返回实际检测到的帧数
}

bool BirdAnimation::tryManualImageLoad(const std::string& file_path) {
    // 使用项目的SD卡接口
    File file = SD.open(file_path.c_str());
    if (!file) {
        return false;
    }

    size_t file_size = file.size();
    
    if (file_size < 32) { // LVGL 9.x最小头部大小
        file.close();
        return false;
    }

    // 读取LVGL 9.x头部
    uint32_t header_cf, flags, stride, reserved_2, data_size;
    uint16_t width, height;

    if (file.read((uint8_t*)&header_cf, sizeof(header_cf)) != sizeof(header_cf) ||
        file.read((uint8_t*)&flags, sizeof(flags)) != sizeof(flags) ||
        file.read((uint8_t*)&width, sizeof(width)) != sizeof(width) ||
        file.read((uint8_t*)&height, sizeof(height)) != sizeof(height) ||
        file.read((uint8_t*)&stride, sizeof(stride)) != sizeof(stride) ||
        file.read((uint8_t*)&reserved_2, sizeof(reserved_2)) != sizeof(reserved_2) ||
        file.read((uint8_t*)&data_size, sizeof(data_size)) != sizeof(data_size)) {
        LOG_ERROR("BIRD", "Failed to read LVGL 9.x header");
        file.close();
        return false;
    }

    // 验证LVGL 9.x文件格式
    uint8_t color_format = header_cf & 0xFF;
    uint8_t magic = (header_cf >> 24) & 0xFF;

    if (color_format != 0x12) { // LVGL 9.x: LV_COLOR_FORMAT_RGB565 = 0x12
        LOG_ERROR("BIRD", "Invalid color format: 0x" + String(color_format, HEX));
        file.close();
        return false;
    }

    if (magic != 0x37) { // LVGL 9.x magic number
        LOG_ERROR("BIRD", "Invalid magic number: 0x" + String(magic, HEX));
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
    LOG_DEBUG("ANIM", "Loading " + String(file_path.c_str()) + ", file_size: " + String(data_size) + ", free_heap: " + String(free_heap));

    if (free_heap < data_size + 4096) { // 预留4KB额外空间
        LOG_ERROR("ANIM", "Insufficient memory - need " + String(data_size) + " + 4096, have " + String(free_heap));
        file.close();
        return false;
    }

    // 分配内存
    lv_image_dsc_t* img_dsc = static_cast<lv_image_dsc_t*>(malloc(sizeof(lv_image_dsc_t)));
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

    // 读取像素数据 - 分块读取以避免长时间阻塞
    const size_t CHUNK_SIZE = 4096; // 每次读取4KB
    size_t bytes_read = 0;
    size_t remaining = data_size;
    uint8_t* write_ptr = img_data;
    
    while (remaining > 0) {
        size_t to_read = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;
        size_t chunk_read = file.read(write_ptr, to_read);
        
        if (chunk_read != to_read) {
            LOG_ERROR("BIRD", "Failed to read chunk: " + String(chunk_read) + "/" + String(to_read));
            file.close();
            free(img_dsc);
            free(img_data);
            return false;
        }
        
        bytes_read += chunk_read;
        write_ptr += chunk_read;
        remaining -= chunk_read;
        
        // 让出CPU，允许看门狗任务运行
        vTaskDelay(1); // 1ms延迟
    }
    
    file.close();

    if (bytes_read != data_size) {
        LOG_ERROR("BIRD", "Failed to read pixel data: " + String(bytes_read) + "/" + String(data_size));
        free(img_dsc);
        free(img_data);
        return false;
    }

    // 设置LVGL图像描述符 - LVGL 9.x格式
    img_dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
    img_dsc->header.cf = color_format;  // 0x12 for RGB565
    img_dsc->header.flags = 0;
    img_dsc->header.w = width;
    img_dsc->header.h = height;
    img_dsc->header.stride = width * 2;  // RGB565每像素2字节
    img_dsc->header.reserved_2 = 0;
    img_dsc->data_size = data_size;
    img_dsc->data = img_data;

    
    // 释放前一帧的内存
    releasePreviousFrame();

    // 保存当前帧的引用
    current_img_dsc_ = img_dsc;
    current_img_data_ = img_data;

    // 设置图像源
    lv_image_set_src(display_obj_, img_dsc);  // LVGL 9.x: lv_img_set_src → lv_image_set_src

    // 计算缩放比例 - canvas现在是240x240，图像是120x120，需要2倍缩放
    // LVGL缩放：256 = 1.0x, 512 = 2.0x
    uint16_t zoom_factor = 512; // 2.0x缩放

    // 设置缩放中心点为图像中心
    lv_img_set_pivot(display_obj_, width / 2, height / 2);

    // 应用缩放
    lv_img_set_zoom(display_obj_, zoom_factor);

    // 设置图像位置到canvas中心
    lv_obj_center(display_obj_);

    // 确保对象可见 - LVGL 9.x: lv_obj_set_hidden被移除，使用add_state/clear_state
    lv_obj_clear_flag(display_obj_, LV_OBJ_FLAG_HIDDEN);

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
    // 创建一个120x120的红色测试图像
    const int width = 120;
    const int height = 120;
    const size_t data_size = width * height * 2; // RGB565

    // 释放前一帧
    releasePreviousFrame();

    // 分配内存
    lv_image_dsc_t* img_dsc = static_cast<lv_image_dsc_t*>(malloc(sizeof(lv_image_dsc_t)));
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

    // 设置图像描述符 - LVGL 9.x格式
    img_dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
    img_dsc->header.cf = 0x12; // LV_COLOR_FORMAT_RGB565
    img_dsc->header.flags = 0;
    img_dsc->header.w = width;
    img_dsc->header.h = height;
    img_dsc->header.stride = width * 2;
    img_dsc->header.reserved_2 = 0;
    img_dsc->data_size = data_size;
    img_dsc->data = img_data;

    // 保存引用
    current_img_dsc_ = img_dsc;
    current_img_data_ = img_data;

    // 设置到显示对象
    lv_image_set_src(display_obj_, img_dsc);  // LVGL 9.x: lv_img_set_src → lv_image_set_src

    // 拉伸到全屏显示
    lv_obj_set_pos(display_obj_, 0, 0);
    lv_obj_set_size(display_obj_, 240, 240); // 拉伸到全屏
}

void BirdAnimation::timerCallback(lv_timer_t* timer) {
    BirdAnimation* animation = static_cast<BirdAnimation*>(lv_timer_get_user_data(timer));
    if (!animation || !animation->is_playing_) {
        return;
    }

    // LVGL定时器在UI任务中执行,已经持有互斥锁
    // 直接调用播放下一帧
    animation->playNextFrame();
}

} // namespace BirdWatching