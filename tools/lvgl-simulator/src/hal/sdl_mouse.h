/**
 * @file sdl_mouse.h
 * SDL3鼠标输入驱动接口
 */

#ifndef SDL_MOUSE_H
#define SDL_MOUSE_H

#include "lvgl/lvgl.h"
#include <SDL3/SDL.h>
#include <stdbool.h>

/**
 * 初始化SDL鼠标驱动
 */
void sdl_mouse_init(void);

/**
 * LVGL输入设备读取回调
 * @param indev_drv 输入设备驱动
 * @param data 输入数据
 * @return 总是返回false（没有缓冲数据）
 */
bool sdl_mouse_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);

#endif /* SDL_MOUSE_H */
