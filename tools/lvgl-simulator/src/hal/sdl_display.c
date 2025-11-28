/**
 * @file sdl_display.c
 * SDL3显示驱动实现
 */

#include "sdl_display.h"
#include <stdio.h>
#include <stdlib.h>

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;
static int window_width = 0;
static int window_height = 0;

/**
 * 初始化SDL显示
 */
bool sdl_display_init(int width, int height)
{
    window_width = width;
    window_height = height;
    
    // 初始化SDL3
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return false;
    }
    
    // 创建窗口
    window = SDL_CreateWindow(
        "LVGL Simulator - RGB565 Image Viewer",
        width, height,
        SDL_WINDOW_RESIZABLE
    );
    
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }
    
    // 创建渲染器
    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }
    
    // 创建纹理 - 使用标准RGB565格式
    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGB565,
        SDL_TEXTUREACCESS_STREAMING,
        width, height
    );
    
    if (!texture) {
        fprintf(stderr, "SDL_CreateTexture Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }
    
    // 清屏
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    
    printf("SDL Display initialized: %dx%d\n", width, height);
    return true;
}

/**
 * 清理SDL显示资源
 */
void sdl_display_deinit(void)
{
    if (texture) {
        SDL_DestroyTexture(texture);
        texture = NULL;
    }
    
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }
    
    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
    
    SDL_Quit();
}

/**
 * LVGL显示刷新回调
 */
void sdl_display_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    if (!texture || !renderer) {
        lv_disp_flush_ready(disp_drv);
        return;
    }
    
    // 计算区域尺寸
    int32_t w = area->x2 - area->x1 + 1;
    int32_t h = area->y2 - area->y1 + 1;
    
    // 更新纹理 - 直接传递LVGL的数据给SDL
    SDL_Rect rect;
    rect.x = area->x1;
    rect.y = area->y1;
    rect.w = w;
    rect.h = h;
    
    SDL_UpdateTexture(texture, &rect, color_p, w * sizeof(lv_color_t));
    
    // 渲染
    SDL_RenderClear(renderer);
    SDL_RenderTexture(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    
    // 通知LVGL刷新完成
    lv_disp_flush_ready(disp_drv);
}

/**
 * 处理SDL事件
 */
void sdl_display_process_events(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                printf("Quit event received\n");
                sdl_display_deinit();
                exit(0);
                break;
                
            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_ESCAPE) {
                    printf("ESC pressed, exiting\n");
                    sdl_display_deinit();
                    exit(0);
                }
                break;
        }
    }
}
