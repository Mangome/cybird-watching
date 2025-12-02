#ifndef BIRD_ANIMATION_H
#define BIRD_ANIMATION_H

#include "bird_types.h"
#include <string>

namespace BirdWatching {

class BirdAnimation {
public:
    BirdAnimation();
    ~BirdAnimation();

    // 初始化动画系统
    bool init(lv_obj_t* parent_obj = nullptr);

    // 加载小鸟动画
    bool loadBird(const BirdInfo& bird_info);

    // 开始循环播放动画
    void startLoop();

    // 停止当前动画
    void stop();

    // 检查是否正在播放
    bool isPlaying() const { return is_playing_; }

    // 获取当前小鸟信息
    const BirdInfo& getCurrentBird() const { return current_bird_; }

    // 设置显示对象
    void setDisplayObject(lv_obj_t* obj);

    // 设置帧间间隔时间
    void setFrameInterval(uint32_t interval_ms) { frame_interval_ = interval_ms; }

    // 获取当前帧间间隔时间
    uint32_t getFrameInterval() const { return frame_interval_; }

    // 检测小鸟的帧数（通过扫描目录）
    uint8_t detectFrameCount(uint16_t bird_id) const;

private:
    lv_obj_t* display_obj_;      // LVGL显示对象
    BirdInfo current_bird_;      // 当前小鸟信息
    uint8_t current_frame_;      // 当前帧
    uint8_t current_frame_count_; // 当前小鸟的实际帧数
    lv_timer_t* play_timer_;      // 播放定时器 (LVGL 9.x: lv_task_t → lv_timer_t)
    bool is_playing_;            // 播放状态
        uint32_t frame_interval_;    // 帧间间隔时间（毫秒，处理完图片后的等待时间）
    bool frame_processing_;      // 当前是否正在处理帧
    uint32_t last_frame_time_;   // 上一帧处理完成的时间

    // 内存管理
    lv_image_dsc_t* current_img_dsc_; // 当前图像描述符 (LVGL 9.x: lv_img_dsc_t → lv_image_dsc_t)
    uint8_t* current_img_data_;     // 当前图像数据

    // 释放前一帧的内存
    void releasePreviousFrame();

    // 创建测试图像（调试用）
    void createTestImage();

    // 定时器回调函数
    static void timerCallback(lv_timer_t* timer);

    // 标志：是否在UI任务中运行
    bool running_in_ui_task_;

    // 获取帧文件路径
    std::string getFramePath(uint8_t frame_index) const;

    // 加载并显示指定帧
    bool loadAndShowFrame(uint8_t frame_index);

    // 播放下一帧
    void playNextFrame();

    // 计划下一帧播放
    void scheduleNextFrame();

    // 手动加载图像（LVGL无法直接加载时使用）
    bool tryManualImageLoad(const std::string& file_path);
};

} // namespace BirdWatching

#endif // BIRD_ANIMATION_H