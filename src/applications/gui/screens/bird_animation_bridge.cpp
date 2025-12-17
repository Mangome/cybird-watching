/**
 * Bird Animation C Bridge
 * 为C文件提供Bird Animation系统的C接口
 */

#include "bird_animation_bridge.h"

#include <Arduino.h>

#include "applications/modules/bird_watching/core/bird_watching.h"

// Bird Animation C接口实现
extern "C" {

bool bird_animation_load_image_to_canvas(lv_obj_t* canvas, uint16_t bird_id, uint8_t frame_index)
{
    try {
        // 使用BirdManager来触发小鸟动画
        // 这样可以统一管理所有小鸟动画，避免多个实例冲突
        return BirdWatching::triggerBird();

    } catch(...) {
        return false;
    }
}

}  // extern "C"