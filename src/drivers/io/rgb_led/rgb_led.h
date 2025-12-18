#ifndef RGB_H
#define RGB_H

#include <FastLED.h>
#include "config/hardware_config.h"

// 最大LED数量 (用于静态数组)
#define MAX_RGB_LEDS 4


class Pixel
{
private:
	CRGB color_buffers[MAX_RGB_LEDS];
	int led_count;  // 实际LED数量
	
	// 通用闪烁方法
	void flash(uint8_t r, uint8_t g, uint8_t b, int duration_ms);

public:
	void init();

	Pixel& setRGB(int id, int r, int g, int b);
	Pixel& setBrightness(float duty);
	
	// 闪烁效果（非阻塞）
	void flashBlue(int duration_ms = 100);
	void flashGreen(int duration_ms = 100);
};

#endif