#include "rgb_led.h"

// 临时禁用FastLED以解决ESP32-S3编译问题
// TODO: 更新FastLED库或使用新的ESP32 RMT API

void Pixel::init()
{
	Serial.println("[RGB_LED] Temporarily disabled - FastLED API deprecated");
	led_count = 0;
}

Pixel& Pixel::setRGB(int id, int r, int g, int b)
{
	// 功能临时禁用
	return *this;
}

Pixel& Pixel::setBrightness(float duty)
{
	// 功能临时禁用
	return *this;
}

void Pixel::flash(uint8_t r, uint8_t g, uint8_t b, int duration_ms)
{
	// 功能临时禁用
}

void Pixel::flashBlue(int duration_ms)
{
	flash(0, 0, 255, duration_ms);
}

void Pixel::flashGreen(int duration_ms)
{
	flash(0, 255, 0, duration_ms);
}
