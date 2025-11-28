/**
 * @file main.c
 * LVGL Windows模拟器主程序
 * 用于预览RGB565格式图片
 */

#include "lvgl/lvgl.h"
#include "hal/sdl_display.h"
#include "hal/sdl_mouse.h"
#include "hal/file_loader.h"
#include <stdio.h>
#include <stdlib.h>

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600
#define DISPLAY_HOR_RES 240
#define DISPLAY_VER_RES 240

// LVGL显示缓冲区
static lv_disp_buf_t disp_buf;
static lv_color_t buf[WINDOW_WIDTH * 10];

// UI对象
static lv_obj_t *img_obj = NULL;
static lv_obj_t *label_info = NULL;
static lv_obj_t *btn_prev = NULL;
static lv_obj_t *btn_next = NULL;
static lv_obj_t *btn_load = NULL;

// 当前显示的图片
static lv_img_dsc_t current_img;
static int current_frame = 0;
static int total_frames = 0;
static char image_path[256] = "./assets/test.bin";

/**
 * 更新信息标签
 */
static void update_info_label(void)
{
    char buf[128];
    if (total_frames > 0) {
        snprintf(buf, sizeof(buf), "Frame: %d/%d | Size: %dx%d | Path: %s",
                 current_frame + 1, total_frames,
                 current_img.header.w, current_img.header.h,
                 image_path);
    } else {
        snprintf(buf, sizeof(buf), "No image loaded");
    }
    lv_label_set_text(label_info, buf);
}

/**
 * 加载并显示图片
 */
static bool load_and_display_image(const char *path)
{
    printf("Loading image: %s\n", path);
    
    if (file_loader_load_bin_image(path, &current_img)) {
        printf("Image loaded successfully: %dx%d\n",
               current_img.header.w, current_img.header.h);
        
        if (img_obj) {
            lv_img_set_src(img_obj, &current_img);
            lv_obj_align(img_obj, NULL, LV_ALIGN_CENTER, 0, 0);
        }
        
        update_info_label();
        return true;
    } else {
        printf("Failed to load image: %s\n", path);
        return false;
    }
}

/**
 * 上一帧按钮回调
 */
static void btn_prev_event_cb(lv_obj_t *btn, lv_event_t event)
{
    if (event == LV_EVENT_CLICKED) {
        if (current_frame > 0) {
            current_frame--;
            char path[256];
            snprintf(path, sizeof(path), "./assets/frame%03d.bin", current_frame);
            load_and_display_image(path);
        }
    }
}

/**
 * 下一帧按钮回调
 */
static void btn_next_event_cb(lv_obj_t *btn, lv_event_t event)
{
    if (event == LV_EVENT_CLICKED) {
        if (current_frame < total_frames - 1) {
            current_frame++;
            char path[256];
            snprintf(path, sizeof(path), "./assets/frame%03d.bin", current_frame);
            load_and_display_image(path);
        }
    }
}

/**
 * 加载图片按钮回调
 */
static void btn_load_event_cb(lv_obj_t *btn, lv_event_t event)
{
    if (event == LV_EVENT_CLICKED) {
        load_and_display_image(image_path);
    }
}

/**
 * 创建UI界面
 */
static void create_ui(void)
{
    // 创建背景容器
    lv_obj_t *cont = lv_cont_create(lv_scr_act(), NULL);
    lv_obj_set_size(cont, WINDOW_WIDTH, WINDOW_HEIGHT);
    lv_obj_align(cont, NULL, LV_ALIGN_CENTER, 0, 0);
    
    // 创建图片显示区域（中心区域）
    lv_obj_t *img_cont = lv_cont_create(cont, NULL);
    lv_obj_set_size(img_cont, DISPLAY_HOR_RES + 20, DISPLAY_VER_RES + 20);
    lv_obj_align(img_cont, NULL, LV_ALIGN_CENTER, 0, -50);
    
    // 创建图片对象
    img_obj = lv_img_create(img_cont, NULL);
    lv_obj_align(img_obj, NULL, LV_ALIGN_CENTER, 0, 0);
    
    // 创建信息标签（顶部）
    label_info = lv_label_create(cont, NULL);
    lv_label_set_long_mode(label_info, LV_LABEL_LONG_SROLL_CIRC);
    lv_obj_set_width(label_info, WINDOW_WIDTH - 40);
    lv_obj_align(label_info, NULL, LV_ALIGN_IN_TOP_MID, 0, 10);
    
    // 创建控制按钮（底部）
    btn_prev = lv_btn_create(cont, NULL);
    lv_obj_set_size(btn_prev, 120, 50);
    lv_obj_align(btn_prev, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 50, -20);
    lv_obj_set_event_cb(btn_prev, btn_prev_event_cb);
    lv_obj_t *label_prev = lv_label_create(btn_prev, NULL);
    lv_label_set_text(label_prev, "< Prev");
    
    btn_load = lv_btn_create(cont, NULL);
    lv_obj_set_size(btn_load, 120, 50);
    lv_obj_align(btn_load, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -20);
    lv_obj_set_event_cb(btn_load, btn_load_event_cb);
    lv_obj_t *label_load = lv_label_create(btn_load, NULL);
    lv_label_set_text(label_load, "Load");
    
    btn_next = lv_btn_create(cont, NULL);
    lv_obj_set_size(btn_next, 120, 50);
    lv_obj_align(btn_next, NULL, LV_ALIGN_IN_BOTTOM_RIGHT, -50, -20);
    lv_obj_set_event_cb(btn_next, btn_next_event_cb);
    lv_obj_t *label_next = lv_label_create(btn_next, NULL);
    lv_label_set_text(label_next, "Next >");
    
    update_info_label();
}

/**
 * 主函数
 */
int main(int argc, char **argv)
{
    printf("LVGL Windows Simulator\n");
    printf("======================\n");
    
    // 检查命令行参数
    if (argc > 1) {
        strncpy(image_path, argv[1], sizeof(image_path) - 1);
    }
    
    // 初始化LVGL
    lv_init();
    
    // 初始化SDL显示驱动
    if (!sdl_display_init(WINDOW_WIDTH, WINDOW_HEIGHT)) {
        fprintf(stderr, "Failed to initialize SDL display\n");
        return 1;
    }
    
    // 初始化SDL鼠标驱动
    sdl_mouse_init();
    
    // 注册显示驱动
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    lv_disp_buf_init(&disp_buf, buf, NULL, WINDOW_WIDTH * 10);
    disp_drv.buffer = &disp_buf;
    disp_drv.flush_cb = sdl_display_flush;
    lv_disp_drv_register(&disp_drv);
    
    // 注册输入设备驱动
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = sdl_mouse_read;
    lv_indev_drv_register(&indev_drv);
    
    // 创建UI
    create_ui();
    
    // 尝试加载默认图片
    total_frames = 1;
    load_and_display_image(image_path);
    
    printf("\nSimulator started. Press Ctrl+C to exit.\n");
    printf("Usage: %s [image_path.bin]\n\n", argv[0]);
    
    // 主循环
    while (1) {
        lv_task_handler();
        sdl_display_process_events();
        SDL_Delay(5);
    }
    
    return 0;
}
