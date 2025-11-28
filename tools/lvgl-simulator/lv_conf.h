/**
 * @file lv_conf.h
 * LVGL配置文件 for Windows Simulator
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   图形设置
 *====================*/

/* 最大水平和垂直分辨率 */
#define LV_HOR_RES_MAX          (800)
#define LV_VER_RES_MAX          (600)

/* 颜色深度: 16 = RGB565 */
#define LV_COLOR_DEPTH     16

/* RGB565字节交换 */
#define LV_COLOR_16_SWAP   0

/* 屏幕透明度 */
#define LV_COLOR_SCREEN_TRANSP    0

/* 透明色 */
#define LV_COLOR_TRANSP    LV_COLOR_LIME

/* 抗锯齿 */
#define LV_ANTIALIAS        1

/* 默认刷新周期 */
#define LV_DISP_DEF_REFR_PERIOD      30      /*[ms]*/

/* DPI */
#define LV_DPI              100

/* 显示尺寸类别 */
#define LV_DISP_SMALL_LIMIT  30
#define LV_DISP_MEDIUM_LIMIT 50
#define LV_DISP_LARGE_LIMIT  70

/* 坐标类型 */
typedef int16_t lv_coord_t;

/*=========================
   内存管理设置
 *=========================*/

/* 使用标准库malloc/free */
#define LV_MEM_CUSTOM      1
#if LV_MEM_CUSTOM == 0
#  define LV_MEM_SIZE    (128U * 1024U)
#  define LV_MEM_ATTR
#  define LV_MEM_ADR          0
#  define LV_MEM_AUTO_DEFRAG  1
#else
#  define LV_MEM_CUSTOM_INCLUDE <stdlib.h>
#  define LV_MEM_CUSTOM_ALLOC   malloc
#  define LV_MEM_CUSTOM_FREE    free
#endif

/* 使用标准memcpy和memset */
#define LV_MEMCPY_MEMSET_STD    1

/* 垃圾回收 */
#define LV_ENABLE_GC 0

/*=======================
   输入设备设置
 *=======================*/

#define LV_INDEV_DEF_READ_PERIOD          30
#define LV_INDEV_DEF_DRAG_LIMIT           10
#define LV_INDEV_DEF_DRAG_THROW           10
#define LV_INDEV_DEF_LONG_PRESS_TIME      400
#define LV_INDEV_DEF_LONG_PRESS_REP_TIME  100
#define LV_INDEV_DEF_GESTURE_LIMIT        50
#define LV_INDEV_DEF_GESTURE_MIN_VELOCITY 3

/*==================
 * 功能使用
 *==================*/

/* 动画 */
#define LV_USE_ANIMATION        1
#if LV_USE_ANIMATION
#define LV_ANIM_DEF_TIME     500
#endif

/* 阴影 */
#define LV_USE_SHADOW           1
#if LV_USE_SHADOW
#define LV_SHADOW_CACHE_SIZE    0
#endif

/* 混合模式 */
#define LV_USE_BLEND_MODES      1

/* 透明度 */
#define LV_USE_OPA_SCALE        1

/* 图片 */
#define LV_USE_IMG_TRANSFORM    1

/* 组 */
#define LV_USE_GROUP            1

/* GPU */
#define LV_USE_GPU              0

/* 文件系统 */
#define LV_USE_FILESYSTEM       1
#if LV_USE_FILESYSTEM
#define LV_FS_PC_PATH         "./assets"
#endif

/*================
 * 日志设置
 *================*/

#define LV_USE_LOG      1
#if LV_USE_LOG
#define LV_LOG_LEVEL    LV_LOG_LEVEL_INFO
#define LV_LOG_PRINTF   1
#endif

/*===================
 *  HAL settings
 *==================*/

/* 1: use a custom tick source.
 * It removes the need to manually update the tick with `lv_tick_inc`) */
#define LV_TICK_CUSTOM     0
#if LV_TICK_CUSTOM == 1
#define LV_TICK_CUSTOM_INCLUDE  <stdint.h>         /*Header for the system time function*/
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (0)           /*Expression evaluating to current system time in ms*/
#endif   /*LV_TICK_CUSTOM*/

typedef void * lv_disp_drv_user_data_t;             /*Type of user data in the display driver*/
typedef void * lv_indev_drv_user_data_t;            /*Type of user data in the input device driver*/

/*================
 * 主题
 *================*/

#define LV_USE_THEME_EMPTY       1
#define LV_USE_THEME_TEMPLATE    1
#define LV_USE_THEME_MATERIAL    1
#define LV_USE_THEME_MONO        1

/*==================
 * 字体
 *==================*/

/* Montserrat fonts with bpp = 4 */
#define LV_FONT_MONTSERRAT_8     0
#define LV_FONT_MONTSERRAT_10    0
#define LV_FONT_MONTSERRAT_12    0
#define LV_FONT_MONTSERRAT_14    1
#define LV_FONT_MONTSERRAT_16    1
#define LV_FONT_MONTSERRAT_18    0
#define LV_FONT_MONTSERRAT_20    0
#define LV_FONT_MONTSERRAT_22    0
#define LV_FONT_MONTSERRAT_24    1
#define LV_FONT_MONTSERRAT_26    0
#define LV_FONT_MONTSERRAT_28    0
#define LV_FONT_MONTSERRAT_30    0
#define LV_FONT_MONTSERRAT_32    0
#define LV_FONT_MONTSERRAT_34    0
#define LV_FONT_MONTSERRAT_36    0
#define LV_FONT_MONTSERRAT_38    0
#define LV_FONT_MONTSERRAT_40    0
#define LV_FONT_MONTSERRAT_42    0
#define LV_FONT_MONTSERRAT_44    0
#define LV_FONT_MONTSERRAT_46    0
#define LV_FONT_MONTSERRAT_48    0

/* 默认字体 */
#define LV_FONT_DEFAULT        &lv_font_montserrat_16

/*===================
 * 文本设置
 *===================*/

#define LV_TXT_ENC LV_TXT_ENC_UTF8
#define LV_TXT_BREAK_CHARS                  " ,.;:-_"
#define LV_TXT_LINE_BREAK_LONG_LEN          0
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN  3
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3
#define LV_TXT_COLOR_CMD "#"
#define LV_USE_BIDI         0
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/*=================
 * 控件使用
 *=================*/

#define LV_USE_ARC          1
#define LV_USE_BAR          1
#define LV_USE_BTN          1
#define LV_USE_BTNMATRIX    1
#define LV_USE_CALENDAR     0
#define LV_USE_CANVAS       1
#define LV_USE_CHECKBOX     1
#define LV_USE_CHART        0
#define LV_USE_CONT         1
#define LV_USE_CPICKER      0
#define LV_USE_DROPDOWN     1
#define LV_USE_GAUGE        0
#define LV_USE_IMG          1
#define LV_USE_IMGBTN       1
#define LV_USE_KEYBOARD     0
#define LV_USE_LABEL        1
#if LV_USE_LABEL
#define LV_LABEL_TEXT_SEL      1
#define LV_LABEL_LONG_TXT_HINT 1
#endif

#define LV_USE_LED          1
#define LV_USE_LINE         1
#define LV_USE_LIST         1
#define LV_USE_LINEMETER    0
#define LV_USE_OBJMASK      1
#define LV_USE_MSGBOX       1
#define LV_USE_PAGE         1
#define LV_USE_SPINNER      1
#define LV_USE_ROLLER       1
#define LV_USE_SLIDER       1
#define LV_USE_SPINBOX      0
#define LV_USE_SWITCH       1
#define LV_USE_TEXTAREA     0
#define LV_USE_TABLE        0
#define LV_USE_TABVIEW      1
#define LV_USE_TILEVIEW     1
#define LV_USE_WIN          1

#endif /*LV_CONF_H*/
