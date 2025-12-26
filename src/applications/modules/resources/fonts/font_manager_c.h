#ifndef FONT_MANAGER_C_H
#define FONT_MANAGER_C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lvgl.h>

/**
 * FontManager C 语言接口
 * 
 * 为 C 代码提供字体加载功能，内部调用 C++ FontManager
 */

/**
 * 从 SD 卡加载字体（C 接口）
 * 
 * @param font_name 字体名称（不含扩展名），例如 "notosanssc_16"
 * @return 成功返回字体指针，失败返回 NULL
 * 
 * 示例：
 *   lv_font_t* font = font_manager_load("notosanssc_16");
 *   if (font) {
 *       lv_obj_set_style_text_font(label, font, 0);
 *   }
 */
lv_font_t* font_manager_load(const char* font_name);

/**
 * 销毁字体（C 接口）
 * 
 * @param font_name 字体名称
 */
void font_manager_destroy(const char* font_name);

/**
 * 检查字体是否已加载（C 接口）
 * 
 * @param font_name 字体名称
 * @return 已加载返回 1，否则返回 0
 */
int font_manager_is_loaded(const char* font_name);

#ifdef __cplusplus
}
#endif

#endif // FONT_MANAGER_C_H
