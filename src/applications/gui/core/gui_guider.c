#include "lvgl.h"
#include <stdio.h>
#include "gui_guider.h"

// 定义全局UI对象
lv_ui guider_ui;

void setup_ui(lv_ui* ui)
{
	// 只初始化scenes界面，包含Bird Animation系统
	// 注意：不在这里加载屏幕，由调用者决定何时显示
	setup_screnes(ui);
}
