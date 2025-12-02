#ifndef GUI_GUIDER_H
#define GUI_GUIDER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "config/guider_fonts.h"

	typedef struct
	{
		lv_obj_t* home;
		lv_obj_t* home_cpicker0;
		lv_obj_t* scenes;
		lv_obj_t* scenes_canvas;
		lv_obj_t* scenes_bird_info_label;
	}lv_ui;

	void setup_ui(lv_ui* ui);
	extern lv_ui guider_ui;
	void setup_scr_home(lv_ui* ui);
	void setup_screnes(lv_ui* ui);

#ifdef __cplusplus
}
#endif
#endif