#include "display.h"
#include "lgfx_config.h"
#include "log_manager.h"

// LovyanGFX 实例（替代 TFT_eSPI）
static LGFX lcd;


void my_print(lv_log_level_t level, const char* file, uint32_t line, const char* fun, const char* dsc)
{
	// 使用日志系统重定向 LVGL 日志
	LogManager* logManager = LogManager::getInstance();
	if (logManager) {
		String message = String(file) + "@" + String(line) + " " + String(fun) + "->" + String(dsc);

		switch (level) {
			case LV_LOG_LEVEL_ERROR:
				logManager->error("LVGL", message);
				break;
			case LV_LOG_LEVEL_WARN:
				logManager->warn("LVGL", message);
				break;
			case LV_LOG_LEVEL_INFO:
				logManager->info("LVGL", message);
				break;
			case LV_LOG_LEVEL_TRACE:
			default:
				logManager->debug("LVGL", message);
				break;
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

	lcd.startWrite();
	lcd.setAddrWindow(area->x1, area->y1, w, h);
	lcd.writePixels((lgfx::rgb565_t*)px_map, w * h);
	lcd.endWrite();

	lv_display_flush_ready(disp);
}


void Display::init()
{
	// 背光控制
	pinMode(LCD_BL_PIN, OUTPUT);
	digitalWrite(LCD_BL_PIN, HIGH);

	lv_init();

	LOG_INFO("LCD", "Initializing LovyanGFX display...");

	// LovyanGFX 初始化
	lcd.init();
	lcd.setRotation(4); // 镜像（与原配置一致）
	lcd.setBrightness(255);
	lcd.fillScreen(TFT_BLACK); // 填充黑色
	
	LOG_INFO("LCD", "LovyanGFX initialization successful");
	LOG_INFO("LCD", String("Display: ") + lcd.width() + "x" + lcd.height());

	/* 创建 LVGL 显示 */
	lv_display_t* disp = lv_display_create(240, 240);
	lv_display_set_flush_cb(disp, my_disp_flush);

	/* 设置显示缓冲区 */
	static lv_color_t buf1[240 * 10];
	lv_display_set_buffers(disp, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
	
	// 创建默认黑色屏幕
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
	if (duty > 0.5) {
		digitalWrite(LCD_BL_PIN, HIGH);
	} else {
		digitalWrite(LCD_BL_PIN, LOW);
	}
}
