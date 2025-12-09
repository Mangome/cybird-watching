#ifndef BIRD_ANIMATION_H
#define BIRD_ANIMATION_H

#include "bird_types.h"
#include "bird_bundle_loader.h"
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

private:
    lv_obj_t* display_obj_;      // LVGL显示对象
    BirdInfo current_bird_;      // 当前小鸟信息
    uint16_t current_frame_;     // 当前帧（支持最多65535帧）
    uint16_t current_frame_count_; // 当前小鸟的实际帧数（支持最多65535帧）
    lv_timer_t* play_timer_;      // 播放定时器 (LVGL 9.x: lv_task_t → lv_timer_t)
    bool is_playing_;            // 播放状态
    bool frame_processing_;      // 当前是否正在处理帧
    uint32_t last_frame_time_;   // 上一帧处理完成的时间

    // 内存管理
    lv_image_dsc_t* current_img_dsc_; // 当前图像描述符 (LVGL 9.x: lv_img_dsc_t → lv_image_dsc_t)
    uint8_t* current_img_data_;     // 当前图像数据

    // 双缓冲：预加载下一帧
    lv_image_dsc_t* next_img_dsc_;  // 下一帧图像描述符
    uint8_t* next_img_data_;        // 下一帧图像数据
    bool next_frame_ready_;         // 下一帧是否已准备好
    
    // 预加载统计（用于自适应优化）
    uint8_t preload_fail_count_;    // 连续预加载失败次数
    bool preload_enabled_;          // 是否启用预加载

    // 释放前一帧的内存
    void releasePreviousFrame();

    // 创建测试图像（调试用）
    void createTestImage();

    // 定时器回调函数
    static void timerCallback(lv_timer_t* timer);

    // 标志：是否在UI任务中运行
    bool running_in_ui_task_;

    // Bundle加载器
    BirdBundleLoader bundle_loader_;

    // 获取帧文件路径
    std::string getFramePath(uint16_t frame_index) const;

    // 加载并显示指定帧
    bool loadAndShowFrame(uint16_t frame_index);

    // 播放下一帧
    void playNextFrame();

    // 计划下一帧播放
    void scheduleNextFrame();

    // 手动加载图像（LVGL无法直接加载时使用）
    bool tryManualImageLoad(const std::string& file_path);

    // 预加载图像到缓冲区
    bool preloadFrameToBuffer(uint16_t frame_index, lv_image_dsc_t** out_dsc, uint8_t** out_data);
    
    // 交换当前帧和下一帧缓冲区
    void swapBuffers();
};

} // namespace BirdWatching

#endif // BIRD_ANIMATION_H