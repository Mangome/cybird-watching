#ifndef DISPLAY_H
#define DISPLAY_H

#include <lvgl.h>

// LCD_BL_PWM_CHANNEL 暂时保留，后续可能用于PWM调光
#define LCD_BL_PWM_CHANNEL 0


class Display
{
private:


public:
	void init();
	void routine();
	void setBackLight(float);
};

#endif