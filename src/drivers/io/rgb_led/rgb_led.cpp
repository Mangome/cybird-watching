#include "rgb_led.h"

void Pixel::init()
{
    FastLED.addLeds<WS2812, RGB_LED_PIN, GRB>(color_buffers, RGB_LED_NUM);
    // 初始化时默认关闭LED
    FastLED.setBrightness(0);
    // 设置默认颜色为黑色（关闭）
    for(int i = 0; i < RGB_LED_NUM; i++) {
        color_buffers[i] = CRGB(0, 0, 0);
    }
    FastLED.show();
}

Pixel& Pixel::setRGB(int id, int r, int g, int b)
{
    color_buffers[id] = CRGB(r, g, b);
    FastLED.show();

    return *this;
}

Pixel& Pixel::setBrightness(float duty)
{
    duty = constrain(duty, 0, 1);
    FastLED.setBrightness((uint8_t)(255 * duty));
    FastLED.show();

    return *this;
}

void Pixel::flash(uint8_t r, uint8_t g, uint8_t b, int duration_ms)
{
    // 保存当前亮度
    uint8_t prev_brightness = FastLED.getBrightness();

    // 设置颜色并点亮
    FastLED.setBrightness(128);  // 中等亮度
    for(int i = 0; i < RGB_LED_NUM; i++) {
        color_buffers[i] = CRGB(r, g, b);
    }
    FastLED.show();

    // 延时
    delay(duration_ms);

    // 关闭并恢复原亮度
    for(int i = 0; i < RGB_LED_NUM; i++) {
        color_buffers[i] = CRGB(0, 0, 0);
    }
    FastLED.setBrightness(prev_brightness);
    FastLED.show();
}

void Pixel::flashBlue(int duration_ms)
{
    flash(0, 0, 255, duration_ms);
}

void Pixel::flashGreen(int duration_ms)
{
    flash(0, 255, 0, duration_ms);
}
