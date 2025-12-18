#include "display.h"
#include "log_manager.h"

/*
Display driver using LovyanGFX
Replaces TFT_eSPI with same interface
*/

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// ESP32-S3 使用 SPI3_HOST，ESP32 使用 VSPI_HOST
#if defined(CONFIG_IDF_TARGET_ESP32S3)
  #define LGFX_SPI_HOST SPI3_HOST
#else
  #define LGFX_SPI_HOST VSPI_HOST
#endif

// LovyanGFX 配置类 - 使用 HardwareConfig 获取引脚
#include "config/hardware_config.h"

class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_SPI _bus_instance;

public:
    LGFX(void)
    {
        // SPI 总线配置 - 从 HardwareConfig 获取引脚
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host = LGFX_SPI_HOST;
            cfg.spi_mode = 3;             // TFT_eSPI 使用 MODE3
            cfg.freq_write = 40000000;
            cfg.freq_read  = 16000000;
            cfg.spi_3wire  = true;
            cfg.use_lock   = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk = HardwareConfig::getPinTFT_SCLK();
            cfg.pin_mosi = HardwareConfig::getPinTFT_MOSI();
            cfg.pin_miso = HardwareConfig::getPinTFT_MISO();
            cfg.pin_dc   = HardwareConfig::getPinTFT_DC();
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        // Panel 配置
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs   = HardwareConfig::getPinTFT_CS();
            cfg.pin_rst  = HardwareConfig::getPinTFT_RST();
            cfg.pin_busy = -1;
            cfg.memory_width  = 240;
            cfg.memory_height = 240;
            cfg.panel_width   = 240;
            cfg.panel_height  = 240;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.offset_rotation = 0;
            cfg.invert      = true;
            cfg.rgb_order   = false;  // 使用 MADCTL 控制颜色顺序
            cfg.dlen_16bit  = false;
            cfg.bus_shared  = false;
            _panel_instance.config(cfg);
        }

        setPanel(&_panel_instance);
    }
};

static LGFX tft;

void my_print(lv_log_level_t level, const char* file, uint32_t line, const char* fun, const char* dsc)
{
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
			default:
				logManager->debug("LVGL", message);
				break;
		}
	} else {
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
	tft.pushPixels((uint16_t*)px_map, w * h, true);
	tft.endWrite();

	lv_display_flush_ready(disp);
}

void Display::init()
{
	pinMode(LCD_BL_PIN, OUTPUT);
	digitalWrite(LCD_BL_PIN, HIGH);

	lv_init();

	LOG_INFO("TFT", "Initializing LovyanGFX...");

	tft.init();
	tft.setRotation(0);
	tft.setColorDepth(16);
	// MADCTL: MX=1(水平镜像) + BGR=1(颜色顺序) = 0x48
	// 与 TFT_eSPI rotation 0 时的 TFT_MAD_COLOR_ORDER(0x08) + MX(0x40) 一致
	tft.writeCommand(0x36);  // MADCTL
	tft.writeData(0x48);     // MX=1, BGR=1
	tft.fillScreen(TFT_BLACK);
	
	LOG_INFO("TFT", "LovyanGFX initialization successful");

	lv_display_t* disp = lv_display_create(240, 240);
	lv_display_set_flush_cb(disp, my_disp_flush);

	static lv_color_t buf1[240 * 10];
	lv_display_set_buffers(disp, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
	
	lv_obj_t* black_scr = lv_obj_create(NULL);
	lv_obj_set_style_bg_color(black_scr, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(black_scr, LV_OPA_COVER, 0);
	lv_scr_load(black_scr);
}

void Display::routine()
{
    lv_tick_inc(1);
    lv_timer_handler();
}

void Display::setBackLight(float duty)
{
	if (duty > 0.5) {
		digitalWrite(LCD_BL_PIN, HIGH);
	} else {
		digitalWrite(LCD_BL_PIN, LOW);
	}
}
