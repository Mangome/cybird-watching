#include "bird_animation.h"
#include "bird_utils.h"
#include "system/logging/log_manager.h"
#include "hal/sd_interface.h"
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
    , frame_processing_(false)
    , current_img_dsc_(nullptr)
    , current_img_data_(nullptr)
    , next_img_dsc_(nullptr)
    , next_img_data_(nullptr)
    , next_frame_ready_(false)
    , preload_fail_count_(0)
    , preload_enabled_(true)
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

    // 构建bundle文件路径
    char bundle_path[64];
    snprintf(bundle_path, sizeof(bundle_path), "/birds/%d/bundle.bin", bird_info.id);

    // 加载bundle文件（必需，无后备方案）
    if (!bundle_loader_.loadBundle(bundle_path)) {
        LOG_ERROR("ANIM", "Failed to load bundle: " + String(bundle_path));
        return false;
    }

    // 从bundle获取帧数
    current_frame_count_ = bundle_loader_.getFrameCount();
    LOG_INFO("ANIM", "Bundle loaded: " + String(current_frame_count_) + " frames from " + String(bundle_path));

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
    
    if (current_frame_count_ == 0) {
        LOG_ERROR("ANIM", "No frames available for bird " + String(current_bird_.id));
        return;
    }

    // 重置到第一帧
    current_frame_ = 0;
    frame_processing_ = false;
    next_frame_ready_ = false;
    preload_fail_count_ = 0;
    preload_enabled_ = true;  // 25MHz SD卡：启用预加载优化

    // 加载并显示第一帧
    if (!loadAndShowFrame(0)) {
        LOG_ERROR("ANIM", "Failed to load first frame");
        return;
    }

    // 设置第一帧处理完成的时间，确保第一帧显示足够时间
    last_frame_time_ = millis();

    is_playing_ = true;

    // 创建播放定时器，20ms周期检查
    play_timer_ = lv_timer_create(timerCallback, 20, this);
    if (!play_timer_) {
        LOG_ERROR("ANIM", "Failed to create animation timer");
        is_playing_ = false;
        return;
    }

    LOG_INFO("ANIM", "Animation started at 20 FPS (50ms/frame) with 25MHz SD card");
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

std::string BirdAnimation::getFramePath(uint16_t frame_index) const {
    char path[128];
    snprintf(path, sizeof(path), "/birds/%d/%d.bin",
             current_bird_.id, frame_index + 1); // 从1开始编号，格式为1.bin, 2.bin等
    return std::string(path);
}

bool BirdAnimation::loadAndShowFrame(uint16_t frame_index) {
    if (!display_obj_) {
        LOG_ERROR("ANIM", "Display object not set");
        return false;
    }

    if (frame_index >= current_frame_count_) {
        LOG_ERROR("ANIM", "Frame index " + String(frame_index) + " out of range");
        return false;
    }

    // 释放前一帧
    releasePreviousFrame();

    // 从bundle加载帧
    lv_image_dsc_t* img_dsc = nullptr;
    uint8_t* img_data = nullptr;

    if (!bundle_loader_.loadFrame(frame_index, &img_dsc, &img_data)) {
        LOG_ERROR("ANIM", "Failed to load frame " + String(frame_index) + " from bundle");
        return false;
    }

    if (!img_dsc || !img_data) {
        LOG_ERROR("ANIM", "Bundle loader returned null pointers");
        return false;
    }

    // 保存当前帧引用
    current_img_dsc_ = img_dsc;
    current_img_data_ = img_data;

    // 设置图像源
    lv_image_set_src(display_obj_, img_dsc);

    // 计算缩放比例 - canvas是240x240，图像是120x120，需要2倍缩放
    // LVGL缩放：256 = 1.0x, 512 = 2.0x
    uint16_t zoom_factor = 512; // 2.0x缩放

    // 设置缩放中心点为图像中心
    lv_img_set_pivot(display_obj_, img_dsc->header.w / 2, img_dsc->header.h / 2);

    // 应用缩放
    lv_img_set_zoom(display_obj_, zoom_factor);

    // 设置图像位置到canvas中心
    lv_obj_center(display_obj_);

    // 确保对象可见
    lv_obj_clear_flag(display_obj_, LV_OBJ_FLAG_HIDDEN);

    // 强制刷新LVGL显示
    lv_obj_invalidate(display_obj_);

    return true;
}

void BirdAnimation::playNextFrame() {
    if (!is_playing_ || frame_processing_ || !display_obj_) {
        return;
    }

    // 检查是否到了播放下一帧的时间
    // 25MHz SD卡速度：~1.5MB/s，每帧加载~18ms
    // 加上vTaskDelay(1)的10ms，总计约30ms，可以支持30+ FPS
    uint32_t now = millis();
    const uint32_t FRAME_INTERVAL_MS = 66; // 15 FPS - 平衡流畅度和看门狗安全
    
    if (now - last_frame_time_ < FRAME_INTERVAL_MS) {
        // 利用空闲时间预加载下一帧（25MHz SD卡足够快）
        if (preload_enabled_ && !next_frame_ready_) {
            uint16_t next_frame = (current_frame_ + 1) % current_frame_count_;
            
            // 检查剩余时间是否足够预加载（至少需要20ms）
            uint32_t time_left = FRAME_INTERVAL_MS - (now - last_frame_time_);
            if (time_left >= 20) {
                bool success = preloadFrameToBuffer(next_frame, &next_img_dsc_, &next_img_data_);
                if (success && next_img_dsc_ && next_img_data_) {
                    next_frame_ready_ = true;
                    preload_fail_count_ = 0;
                } else {
                    preload_fail_count_++;
                    if (preload_fail_count_ >= 3) {
                        preload_enabled_ = false;
                        Serial.println("[WARN] Preload disabled (memory low)");
                    }
                }
            }
        }
        return;
    }

    // 标记开始处理帧
    frame_processing_ = true;
    uint32_t frame_start = millis();

    current_frame_++;
    if (current_frame_ >= current_frame_count_) {
        current_frame_ = 0;
    }

    // 如果下一帧已预加载，直接使用（双缓冲）
    if (next_frame_ready_ && next_img_dsc_ && next_img_data_) {
        // 释放当前帧
        if (current_img_data_) free(current_img_data_);
        if (current_img_dsc_) free(current_img_dsc_);
        
        // 交换缓冲区
        current_img_dsc_ = next_img_dsc_;
        current_img_data_ = next_img_data_;
        next_img_dsc_ = nullptr;
        next_img_data_ = nullptr;
        next_frame_ready_ = false;
        
        // 显示预加载的帧
        lv_image_set_src(display_obj_, current_img_dsc_);
        
        if (current_img_dsc_) {
            lv_img_set_pivot(display_obj_, current_img_dsc_->header.w / 2, current_img_dsc_->header.h / 2);
            lv_img_set_zoom(display_obj_, 512); // 2.0x
            lv_obj_center(display_obj_);
            lv_obj_clear_flag(display_obj_, LV_OBJ_FLAG_HIDDEN);
        }
        
        // 让出CPU给看门狗任务，防止触发看门狗超时
        vTaskDelay(1); // 延迟1个tick (~10ms)
    } else {
        // 预加载失败或未启用，实时加载
        if (next_img_data_) free(next_img_data_);
        if (next_img_dsc_) free(next_img_dsc_);
        next_img_data_ = nullptr;
        next_img_dsc_ = nullptr;
        next_frame_ready_ = false;
        
        if (!loadAndShowFrame(current_frame_)) {
            stop();
            frame_processing_ = false;
            return;
        }
        
        // 实时加载更耗时，也要让出CPU
        vTaskDelay(1);
    }
    
    uint32_t load_time = millis() - frame_start;
    if (load_time > FRAME_INTERVAL_MS) {
        Serial.printf("[WARN] Frame %d load: %dms (target: %dms)\n", current_frame_, load_time, FRAME_INTERVAL_MS);
    }

    last_frame_time_ = millis();
    frame_processing_ = false;
}


bool BirdAnimation::tryManualImageLoad(const std::string& file_path) {
    // 使用项目的SD卡接口
    fs::FS& fs = HAL::SDInterface::getFS();
    File file = fs.open(file_path.c_str());
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
    // LOG_DEBUG("ANIM", "Loading " + String(file_path.c_str()) + ", file_size: " + String(data_size) + ", free_heap: " + String(free_heap)); // 注释掉以避免日志洪流

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

    // 读取像素数据 - 性能分析
    static uint8_t perf_log_count = 0;
    uint32_t read_start = millis();
    size_t bytes_read = file.read(img_data, data_size);
    uint32_t read_time = millis() - read_start;
    file.close();

    // 前3帧输出性能日志到串口
    if (perf_log_count < 3) {
        Serial.printf("[PERF] SD read %uB in %ums (%u KB/s)\n", 
                      data_size, read_time, data_size * 1000 / read_time / 1024);
        perf_log_count++;
    }

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
    
    // 释放预加载缓冲区
    if (next_img_data_) {
        free(next_img_data_);
        next_img_data_ = nullptr;
    }

    if (next_img_dsc_) {
        free(next_img_dsc_);
        next_img_dsc_ = nullptr;
    }
    
    next_frame_ready_ = false;
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

bool BirdAnimation::preloadFrameToBuffer(uint16_t frame_index, lv_image_dsc_t** out_dsc, uint8_t** out_data) {
    if (!out_dsc || !out_data) {
        LOG_ERROR("ANIM", "Invalid output parameters for preload");
        return false;
    }

    if (frame_index >= current_frame_count_) {
        return false;
    }

    // 使用bundle loader加载帧
    return bundle_loader_.loadFrame(frame_index, out_dsc, out_data);
}

} // namespace BirdWatching