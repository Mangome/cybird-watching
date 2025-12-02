/*********************
 *      INCLUDES
 *********************/
#include "lv_cubic_gui.h"
#include "images.h"

lv_obj_t* scr;

void lv_init_gui(void)
{
	static lv_style_t default_style;
	lv_style_init(&default_style);
	lv_style_set_bg_color(&default_style, lv_color_black());
	lv_style_set_bg_opa(&default_style, LV_OPA_COVER);

	lv_obj_add_style(lv_scr_act(), &default_style, 0);

	scr = lv_scr_act();
	lv_obj_t* img = lv_image_create(lv_scr_act());
	lv_image_set_src(img, &logo);
//	lv_img_set_src(img, "S:/pic.bin");
	lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
}