/**
 * @file sdl_display.h
 * SDL3显示驱动接口
 */

#ifndef SDL_DISPLAY_H
#define SDL_DISPLAY_H

#include "lvgl/lvgl.h"
#include <SDL3/SDL.h>
#include <stdbool.h>

/**
 * 初始化SDL显示
 * @param width 窗口宽度
 * @param height 窗口高度
 * @return 成功返回true
 */
bool sdl_display_init(int width, int height);

/**
 * 清理SDL显示资源
 */
void sdl_display_deinit(void);

/**
 * LVGL显示刷新回调
 * @param disp_drv 显示驱动
 * @param area 刷新区域
 * @param color_p 颜色数据
 */
void sdl_display_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);

/**
 * 处理SDL事件
 */
void sdl_display_process_events(void);

#endif /* SDL_DISPLAY_H */
