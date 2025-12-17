/**
 * Bird Animation C Bridge Header
 * 为C文件提供Bird Animation系统的C接口声明
 */

#ifndef BIRD_ANIMATION_BRIDGE_H
#define BIRD_ANIMATION_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/**
 * 使用Bird Animation系统加载小鸟图片到canvas
 *
 * @param canvas LVGL图像对象
 * @param bird_id 小鸟ID
 * @param frame_index 帧索引（从0开始）
 * @return true 成功，false 失败
 */
bool bird_animation_load_image_to_canvas(lv_obj_t* canvas, uint16_t bird_id, uint8_t frame_index);

#ifdef __cplusplus
}
#endif

#endif  // BIRD_ANIMATION_BRIDGE_H