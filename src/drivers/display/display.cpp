#include "display.h"

#include <TFT_eSPI.h>

#include "log_manager.h"

/*
TFT pins should be set in path/to/Arduino/libraries/TFT_eSPI/User_Setups/Setup24_ST7789.h
*/
TFT_eSPI tft = TFT_eSPI();

void my_print(lv_log_level_t level, const char* file, uint32_t line, const char* fun, const char* dsc)
{
    // 使用日志系统重定向 LVGL 日志
    LogManager* logManager = LogManager::getInstance();
    if(logManager) {
        String message = String(file) + "@" + String(line) + " " + String(fun) + "->" + String(dsc);

        switch(level) {
            case LV_LOG_LEVEL_ERROR: logManager->error("LVGL", message); break;
            case LV_LOG_LEVEL_WARN: logManager->warn("LVGL", message); break;
            case LV_LOG_LEVEL_INFO: logManager->info("LVGL", message); break;
            case LV_LOG_LEVEL_TRACE:
            default: logManager->debug("LVGL", message); break;
        }
    } else {
        // 备用：直接输出到串口
        Serial.printf("%s@%d %s->%s\r\n", file, line, fun, dsc);
        Serial.flush();
    }
}

void my_disp_flush(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t*)px_map, w * h, true);
    tft.endWrite();

    lv_display_flush_ready(disp);
}

void Display::init()
{
    // Temporarily disable PWM backlight to avoid GPIO error
    // ledcSetup(LCD_BL_PWM_CHANNEL, 5000, 8);
    // ledcAttachPin(LCD_BL_PIN, LCD_BL_PWM_CHANNEL);
    pinMode(LCD_BL_PIN, OUTPUT);
    digitalWrite(LCD_BL_PIN, HIGH);

    lv_init();

    // lv_log_register_print_cb() is deprecated in LVGL 9.x

    LOG_INFO("TFT", "Testing minimal TFT init...");

    // Try a minimal TFT initialization
    pinMode(2, OUTPUT);  // DC pin
    digitalWrite(2, HIGH);

    LOG_INFO("TFT", "DC pin set, attempting tft.begin()...");
    tft.begin(); /* TFT init */
    LOG_INFO("TFT", "tft.begin() completed");

    tft.setRotation(4); /* mirror */

    // 立即填充黑色，避免显示未初始化的内容
    tft.fillScreen(TFT_BLACK);

    LOG_INFO("TFT", "TFT initialization successful");

    /* Create the display */
    lv_display_t* disp = lv_display_create(240, 240);
    lv_display_set_flush_cb(disp, my_disp_flush);

    /* Set display buffers - the old draw_buf approach is deprecated */
    static lv_color_t buf1[240 * 10];
    lv_display_set_buffers(disp, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    // 创建并加载一个默认的黑色屏幕，避免LVGL渲染未定义内容
    lv_obj_t* black_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(black_scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(black_scr, LV_OPA_COVER, 0);
    lv_scr_load(black_scr);
}

void Display::routine()
{
    // 更新LVGL系统tick，1ms
    lv_tick_inc(1);

    // 处理LVGL定时器
    lv_timer_handler();
}

void Display::setBackLight(float duty)
{
    // Simple digital backlight control for now
    if(duty > 0.5) {
        digitalWrite(LCD_BL_PIN, HIGH);
    } else {
        digitalWrite(LCD_BL_PIN, LOW);
    }
}
