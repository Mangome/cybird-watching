/*
 * Copyright 2021 NXP
 * SPDX-License-Identifier: MIT
 */

#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"
#include "Arduino.h"
#include "bird_animation_bridge.h"
#include "applications/modules/resources/fonts/font_manager_c.h"

// 小鸟信息显示配置
// ⚠️ 字体配置统一在这里管理，请勿在其他文件重复定义
#define BIRD_INFO_USE_CHINESE_FONT 1  // 1: 使用中文字体, 0: 使用英文字体
#define BIRD_INFO_FONT_SIZE 18  // 字体大小（如需修改，需重新生成对应大小的字体文件）

// 构建字体名称（从 SD 卡加载）
#if BIRD_INFO_USE_CHINESE_FONT
    #if BIRD_INFO_FONT_SIZE == 12
        #define BIRD_INFO_FONT_NAME "notosanssc_12"
    #elif BIRD_INFO_FONT_SIZE == 16
        #define BIRD_INFO_FONT_NAME "notosanssc_16"
    #elif BIRD_INFO_FONT_SIZE == 18
        #define BIRD_INFO_FONT_NAME "notosanssc_18"
    #else
        #define BIRD_INFO_FONT_NAME "notosanssc_16"  // 默认16号
    #endif
    #define FALLBACK_FONT &lv_font_montserrat_14  // 备用字体（使用LVGL已启用的字体）
#else
    #define FALLBACK_FONT &lv_font_montserrat_14  // 备用字体
#endif


void setup_screnes(lv_ui* ui)
{
	//Write codes scenes
	ui->scenes = lv_obj_create(NULL);

	//Write codes scenes_canvas
	ui->scenes_canvas = lv_image_create(ui->scenes);

	//Write style for scenes_canvas (LVGL 9.x中移除了LV_IMG_PART_MAIN)
	static lv_style_t style_scenes_canvas_main;
	lv_style_init(&style_scenes_canvas_main);

	// LVGL 9.x中样式API不再需要状态参数
	lv_style_set_bg_color(&style_scenes_canvas_main, lv_color_black());
	lv_style_set_bg_color(&style_scenes_canvas_main, lv_color_hex(0x888888));
	lv_style_set_bg_color(&style_scenes_canvas_main, lv_color_hex(0x666666));

	//Write style for scenes (LVGL 9.x中移除了LV_BTN_PART_MAIN，改为LV_PART_MAIN)
	lv_obj_add_style(ui->scenes, &style_scenes_canvas_main, LV_PART_MAIN);

	// 触发小鸟动画（通过BirdManager管理，避免多实例问题）
	bird_animation_load_image_to_canvas(ui->scenes_canvas, 1001, 0);

	// 设置对象位置和大小 - 覆盖整个屏幕
	lv_obj_set_size(ui->scenes_canvas, 240, 240); // 设置为全屏大小
	lv_obj_align(ui->scenes_canvas, LV_ALIGN_CENTER, 0, 0);

	// 确保对象可见
	lv_obj_clear_flag(ui->scenes_canvas, LV_OBJ_FLAG_HIDDEN);

	// 创建小鸟信息标签（在右下角显示）
	ui->scenes_bird_info_label = lv_label_create(ui->scenes);
	lv_obj_set_style_text_color(ui->scenes_bird_info_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
	
	// 从 SD 卡动态加载字体
	#if BIRD_INFO_USE_CHINESE_FONT
		lv_font_t* bird_info_font = font_manager_load(BIRD_INFO_FONT_NAME);
		
		if (bird_info_font) {
			// 使用从 SD 卡加载的字体
			lv_obj_set_style_text_font(ui->scenes_bird_info_label, bird_info_font, LV_PART_MAIN);
		} else {
			// 加载失败，使用备用字体
			lv_obj_set_style_text_font(ui->scenes_bird_info_label, FALLBACK_FONT, LV_PART_MAIN);
		}
	#else
		// 使用 LVGL 内置英文字体
		lv_obj_set_style_text_font(ui->scenes_bird_info_label, FALLBACK_FONT, LV_PART_MAIN);
	#endif
	
	lv_label_set_text(ui->scenes_bird_info_label, "");
	lv_obj_align(ui->scenes_bird_info_label, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
	lv_obj_add_flag(ui->scenes_bird_info_label, LV_OBJ_FLAG_HIDDEN); // 默认隐藏
}