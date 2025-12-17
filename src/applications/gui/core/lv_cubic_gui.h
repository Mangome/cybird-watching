#ifndef LV_CUBIC_GUI_H
#define LV_CUBIC_GUI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "gui_guider.h"
#include "lvgl.h"

extern lv_obj_t* scr1;
extern lv_obj_t* scr2;

void lv_init_gui(void);
void lv_check_logo_timeout(void);  // 检查logo是否超时（在UI任务中循环调用）
void lv_hide_logo(void);           // 立即隐藏logo并显示小鸟界面

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
