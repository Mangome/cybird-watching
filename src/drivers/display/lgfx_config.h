/**
 * @file lgfx_config.h
 * @brief LovyanGFX 配置文件 - ST7789 240x240 显示屏
 * 
 * 替代 TFT_eSPI，支持 ESP32 PICO32 和 ESP32-S3
 * 硬件引脚：SCLK=18, MOSI=23, MISO=19, DC=2, RST=4, BL=5
 */

#ifndef LGFX_CONFIG_H
#define LGFX_CONFIG_H

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_SPI _bus_instance;
    lgfx::Light_PWM _light_instance;

public:
    LGFX(void)
    {
        // SPI 总线配置
        {
            auto cfg = _bus_instance.config();

#ifdef CONFIG_IDF_TARGET_ESP32S3
            cfg.spi_host = SPI2_HOST;     // ESP32-S3 使用 SPI2_HOST（FSPI）
#else
            cfg.spi_host = VSPI_HOST;     // ESP32 使用 VSPI_HOST（SPI3）
#endif
            cfg.spi_mode = 3;             // 关键：ST7789需要SPI_MODE3！（TFT_eSPI也使用MODE3）
            cfg.freq_write = 40000000;    // 40MHz
            cfg.freq_read  = 6000000;     // 降低读取频率（参考示例）
            cfg.spi_3wire  = true;
            cfg.use_lock   = true;
            cfg.dma_channel = 1;          // 使用DMA通道1（参考示例，0=禁用）
            
            cfg.pin_sclk = 18;
            cfg.pin_mosi = 23;
            cfg.pin_miso = -1;  // 参考示例设为-1（3线SPI不需要MISO）
            cfg.pin_dc   = 2;

            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        // Panel 配置（参考LingShunLAB示例）
        {
            auto cfg = _panel_instance.config();

            cfg.pin_cs   = -1;  // 未连接
            cfg.pin_rst  = 4;   // Reset引脚
            cfg.pin_busy = -1;  // 未使用

            // 关键：240x240屏幕必须同时设置memory和panel尺寸
            // Panel_ST7789默认是320高度，必须覆盖！
            cfg.memory_width  = 240;
            cfg.memory_height = 240;
            cfg.panel_width   = 240;
            cfg.panel_height  = 240;
            
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.offset_rotation = 0;
            
            // 关键配置（参考示例）
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits  = 1;
            cfg.readable    = true;   // 启用可读模式
            cfg.invert      = true;   // ST7789需要反转
            cfg.rgb_order   = false;  // BGR顺序
            cfg.dlen_16bit  = false;
            cfg.bus_shared  = false;

            _panel_instance.config(cfg);
        }

        // 背光 PWM 配置
        {
            auto cfg = _light_instance.config();

            cfg.pin_bl = 5;
            cfg.invert = false;
            cfg.freq   = 44100;
            cfg.pwm_channel = 0;

            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }

        setPanel(&_panel_instance);
    }
};

#endif // LGFX_CONFIG_H
