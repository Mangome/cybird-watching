/**
 * @file sdl_mouse.c
 * SDL3鼠标输入驱动实现
 */

#include "sdl_mouse.h"
#include <stdio.h>

static float last_x = 0;
static float last_y = 0;
static bool left_button_down = false;

/**
 * 初始化SDL鼠标驱动
 */
void sdl_mouse_init(void)
{
    last_x = 0;
    last_y = 0;
    left_button_down = false;
    printf("SDL Mouse initialized\n");
}

/**
 * LVGL输入设备读取回调
 */
bool sdl_mouse_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    (void)indev_drv; // 未使用的参数
    
    // 获取鼠标状态
    float x, y;
    Uint32 mouse_state = SDL_GetMouseState(&x, &y);
    
    // 检测左键状态
    if (mouse_state & SDL_BUTTON_LMASK) {
        left_button_down = true;
        data->state = LV_INDEV_STATE_PR;
    } else {
        left_button_down = false;
        data->state = LV_INDEV_STATE_REL;
    }
    
    // 更新坐标
    last_x = x;
    last_y = y;
    
    data->point.x = (int)x;
    data->point.y = (int)y;
    
    return false; // 没有缓冲数据
}
