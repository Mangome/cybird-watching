

    #ifndef GUIDER_FONTS_H
    #define GUIDER_FONTS_H
    #ifdef __cplusplus
    extern "C" {
    #endif
    
    #include "lvgl.h"
    
    // 字体已从 SD 卡动态加载，不再需要在固件中声明
    // 使用 FontManager 从 SD 卡加载字体：
    //   lv_font_t* font = FONT_MANAGER.loadFont("notosanssc_16");
    //
    // 已移除的固件内置字体：
    //   - lv_font_notosanssc_12 (~73KB)
    //   - lv_font_notosanssc_16 (~98KB)
    //   - lv_font_notosanssc_18 (~110KB)
    // 节省 Flash 空间: ~281KB

    
    #ifdef __cplusplus
    }
    #endif
    #endif
    