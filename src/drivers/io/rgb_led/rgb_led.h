#ifndef RGB_H
#define RGB_H

#include <FastLED.h>
#include "config/hardware_config.h"
#include <Arduino.h>

// 最大LED数量 (用于静态数组)
#define MAX_RGB_LEDS 4

class Pixel
{
private:
	static CRGB leds[MAX_RGB_LEDS];
	static bool initialized;
	int led_count;  // 实际LED数量

public:
	void init();
	void testSequence();  // 新增：启动测试序列
	void clear();         // 新增：清除所有LED

	Pixel& setRGB(int id, int r, int g, int b);
	Pixel& setBrightness(float duty);
	
	// 通用闪烁方法（公开，用于自定义颜色）
	void flash(uint8_t r, uint8_t g, uint8_t b, int duration_ms);
	
	// 闪烁效果（非阻塞）
	void flashBlue(int duration_ms = 100);
	void flashGreen(int duration_ms = 100);
	void flashRed(int duration_ms = 100);  // 新增：红色闪烁（错误指示）
};

#endif