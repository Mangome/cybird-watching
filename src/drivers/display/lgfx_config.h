/**
 * @file lgfx_config.h
 * @brief LovyanGFX 配置文件 - ST7789 240x240 显示屏
 * 
 * 替代 TFT_eSPI，支持 ESP32 和 ESP32-S3
 */

#ifndef LGFX_CONFIG_H
#define LGFX_CONFIG_H

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_SPI _bus_instance;

public:
    LGFX(void)
    {
        {
            auto cfg = _bus_instance.config();

            // SPI 总线配置
#ifdef CONFIG_IDF_TARGET_ESP32S3
            cfg.spi_host = SPI3_HOST;     // ESP32-S3 使用 SPI3_HOST
#else
            cfg.spi_host = VSPI_HOST;     // ESP32 使用 VSPI_HOST
#endif
            cfg.spi_mode = 0;             // SPI 模式 0
            cfg.freq_write = 40000000;    // 写入频率 40MHz（与原配置一致）
            cfg.freq_read  = 20000000;    // 读取频率 20MHz
            cfg.spi_3wire  = true;        // 3线SPI模式
            cfg.use_lock   = true;        // 使用事务锁
            cfg.dma_channel = SPI_DMA_CH_AUTO; // 自动分配 DMA 通道
            
            // 引脚配置（与 Setup24_ST7789.h 一致）
            cfg.pin_sclk = 18;            // SPI SCLK
            cfg.pin_mosi = 23;            // SPI MOSI
            cfg.pin_miso = 19;            // SPI MISO
            cfg.pin_dc   = 2;             // Data/Command

            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        {
            auto cfg = _panel_instance.config();

            cfg.pin_cs           = -1;    // CS 未连接
            cfg.pin_rst          = 4;     // Reset 引脚
            cfg.pin_busy         = -1;    // Busy 引脚（未使用）

            // 显示分辨率
            cfg.memory_width     = 240;
            cfg.memory_height    = 240;
            cfg.panel_width      = 240;
            cfg.panel_height     = 240;

            cfg.offset_x         = 0;
            cfg.offset_y         = 0;
            cfg.offset_rotation  = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits  = 1;
            cfg.readable         = false;
            cfg.invert           = true;  // ST7789 通常需要反转
            cfg.rgb_order        = false; // BGR 顺序
            cfg.dlen_16bit       = false;
            cfg.bus_shared       = true;

            _panel_instance.config(cfg);
        }

        setPanel(&_panel_instance);
    }
};

#endif // LGFX_CONFIG_H
