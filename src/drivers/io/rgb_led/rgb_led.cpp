#include "rgb_led.h"
#include "config/hardware_config.h"
#include "system/logging/log_manager.h"

// 静态成员定义
CRGB Pixel::leds[MAX_RGB_LEDS];
bool Pixel::initialized = false;

void Pixel::init()
{
	if (initialized) {
		return;
	}

	led_count = MAX_RGB_LEDS;
	
	// 根据平台选择引脚
	#ifdef PLATFORM_ESP32_S3
		int pin = HardwareConfig::ESP32S3Pins::RGB_LED_PIN;
		FastLED.addLeds<WS2812, HardwareConfig::ESP32S3Pins::RGB_LED_PIN, GRB>(leds, MAX_RGB_LEDS);
	#else
		int pin = HardwareConfig::ESP32Pins::RGB_LED_PIN;
		FastLED.addLeds<WS2812, HardwareConfig::ESP32Pins::RGB_LED_PIN, GRB>(leds, MAX_RGB_LEDS);
	#endif
	
	FastLED.setBrightness(50);
	FastLED.clear();
	FastLED.show();
	
	initialized = true;
	LOG_INFO("RGB_LED", String("RGB LED initialized on pin ") + String(pin));
}

void Pixel::testSequence()
{
	if (!initialized) {
		init();
	}
	
	LOG_INFO("RGB_LED", "Running test sequence...");
	
	// 彩虹效果测试（快速）
	for (int i = 0; i < 256; i += 8) {
		fill_rainbow(leds, led_count, i, 7);
		FastLED.show();
		delay(10);
	}
	
	// 闪烁测试
	flashBlue(100);
	delay(200);
	flashGreen(100);
	delay(200);
	clear();
	
	LOG_INFO("RGB_LED", "Test sequence completed");
}

Pixel& Pixel::setRGB(int id, int r, int g, int b)
{
	if (id >= 0 && id < led_count) {
		leds[id] = CRGB(r, g, b);
	}
	return *this;
}

Pixel& Pixel::setBrightness(float duty)
{
	if (duty >= 0.0f && duty <= 1.0f) {
		FastLED.setBrightness(static_cast<uint8_t>(duty * 255));
	}
	return *this;
}

void Pixel::clear()
{
	FastLED.clear();
	FastLED.show();
}

void Pixel::flash(uint8_t r, uint8_t g, uint8_t b, int duration_ms)
{
	// 点亮所有 LED
	for (int i = 0; i < led_count; i++) {
		leds[i] = CRGB(r, g, b);
	}
	FastLED.show();
	
	// 保持指定时间
	if (duration_ms > 0) {
		delay(duration_ms);
		clear();
	}
}

void Pixel::flashBlue(int duration_ms)
{
	flash(0, 0, 255, duration_ms);
}

void Pixel::flashGreen(int duration_ms)
{
	flash(0, 255, 0, duration_ms);
}

void Pixel::flashRed(int duration_ms)
{
	flash(255, 0, 0, duration_ms);
}
