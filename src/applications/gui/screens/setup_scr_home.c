/*
 * Copyright 2021 NXP
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>

#include "gui_guider.h"
#include "lvgl.h"

void setup_scr_home(lv_ui* ui)
{
    // Write codes home
    ui->home = lv_obj_create(NULL);

    // LVGL 9.x中移除了color picker，这里使用一个简单的按钮作为替代
    ui->home_cpicker0 = lv_btn_create(ui->home);

    // Write style for home_cpicker0 (按钮替代color picker)
    static lv_style_t style_home_cpicker0_main;
    lv_style_init(&style_home_cpicker0_main);

    // LVGL 9.x中样式API不再需要状态参数
    lv_style_set_pad_all(&style_home_cpicker0_main, 10);
    lv_style_set_width(&style_home_cpicker0_main, 200);
    lv_style_set_height(&style_home_cpicker0_main, 200);
    lv_style_set_radius(&style_home_cpicker0_main, 100);  // 圆形按钮
    lv_style_set_bg_color(&style_home_cpicker0_main, lv_color_hex(0x888888));
    lv_obj_add_style(ui->home_cpicker0, &style_home_cpicker0_main, LV_PART_MAIN);
    lv_obj_set_pos(ui->home_cpicker0, 15, 16);
}