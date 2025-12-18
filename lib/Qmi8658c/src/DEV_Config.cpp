/*****************************************************************************
* | File      	:   DEV_Config.c
* | Author      :
* | Function    :   Hardware underlying interface
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2021-03-16
* | Info        :
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of theex Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
******************************************************************************/
#include "DEV_Config.h"

uint slice_num;
SPIClass * vspi = NULL;
/**
 * GPIO read and write
 **/
void DEV_Digital_Write(uint16_t Pin, uint8_t Value)
{
    digitalWrite(Pin, Value);
}

uint8_t DEV_Digital_Read(uint16_t Pin)
{
    return digitalRead(Pin);
}

/**
 * SPI
 **/
void DEV_SPI_WriteByte(uint8_t Value)
{
    vspi->transfer(Value);
}

void DEV_SPI_Write_nByte(uint8_t pData[], uint32_t Len)
{
     vspi->transfer(pData, Len);
}

/**
 * I2C
 **/

void DEV_I2C_Write_Byte(uint8_t addr, uint8_t reg, uint8_t Value)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(Value);
    Wire.endTransmission();
}

void DEV_I2C_Write_Register(uint8_t addr, uint8_t reg, uint16_t value)
{
    uint8_t tmpi[3];
    tmpi[0] = reg;
    tmpi[1] = (value >> 8) & 0xFF;
    tmpi[2] = value & 0xFF;
    Wire.beginTransmission(addr);
    Wire.write(tmpi,3);
    Wire.endTransmission();
    
}

void DEV_I2C_Write_nByte(uint8_t addr,uint8_t *pData, uint32_t Len)
{
    Wire.beginTransmission(addr);
    Wire.write(pData,Len);
    Wire.endTransmission();
}

uint8_t DEV_I2C_Read_Byte(uint8_t addr, uint8_t reg)
{
    uint8_t value;
  
    Wire.beginTransmission(addr);
    Wire.write((byte)reg);
    Wire.endTransmission();
  
    Wire.requestFrom(addr, (byte)1);
    value = Wire.read();
  
    return value;
}
void DEV_I2C_Read_Register(uint8_t addr, uint8_t reg, uint16_t *value)
{
    uint8_t tmpi[2];
    
    Wire.beginTransmission(addr);
    Wire.write(reg);
    // Wire.endTransmission();
    Wire.requestFrom(addr, 2);
  
    uint8_t i = 0;
    for(i = 0; i < 2; i++) {
      tmpi[i] =  Wire.read();
    }
    Wire.endTransmission();
    *value = (((uint16_t)tmpi[0] << 8) | (uint16_t)tmpi[1]);
}

void DEV_I2C_Read_nByte(uint8_t addr, uint8_t reg, uint8_t *pData, uint32_t Len)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.endTransmission();
    
    Wire.requestFrom(addr, Len);
  
    uint8_t i = 0;
    for(i = 0; i < Len; i++) {
      pData[i] =  Wire.read();
    }
    Wire.endTransmission();
}

uint16_t DEC_ADC_Read(void)
{
  return analogReadMilliVolts(BAT_ADC_PIN);
    // return analogRead(BAR_CHANNEL);
  // return 0;
}

/**
 * GPIO Mode
 **/
void DEV_GPIO_Mode(uint16_t Pin, uint16_t Mode)
{

    if (Mode == 0 )
    {
        pinMode(Pin, INPUT);
    }
    else
    {
        pinMode(Pin, OUTPUT);
    }
}

/**
 * KEY Config
 **/
void DEV_KEY_Config(uint16_t Pin)
{
    pinMode(Pin,INPUT_PULLUP);
}
/*
** PWM
*/
void DEV_SET_PWM(uint8_t Value)
{
    if (Value < 0 || Value > 100)
    {
        printf("DEV_SET_PWM Error \r\n");
    }
    else
    {
        analogWrite(LCD_BL_PIN, Value * 2.55);
    }
}

/**
 * delay x ms
 **/
void DEV_Delay_ms(uint32_t xms)
{
    delay(xms);
}

void DEV_Delay_us(uint32_t xus)
{
    delayMicroseconds(xus);
}

void DEV_GPIO_Init(void)
{

    DEV_GPIO_Mode(LCD_RST_PIN, 1);
    DEV_GPIO_Mode(LCD_DC_PIN, 1);
    DEV_GPIO_Mode(LCD_CS_PIN, 1);
    DEV_GPIO_Mode(LCD_BL_PIN, 1);
    DEV_GPIO_Mode(Touch_RST_PIN, 1);
    //ADC
//    DEV_GPIO_Mode(BAT_ADC_PIN,0);
    // analogReadResolution(12);
    
    DEV_Digital_Write(LCD_CS_PIN, 1);
    DEV_Digital_Write(LCD_DC_PIN, 0);
    // PWM Config
    // DEV_SET_PWM(0);
    DEV_Digital_Write(LCD_BL_PIN, 1);
}


/******************************************************************************
function:	Module Initialize, the library and initialize the pins, SPI protocol
parameter:
Info:
******************************************************************************/
uint8_t DEV_Module_Init(void)
{
    Serial.begin(115200);
    DEV_Delay_ms(100);
    // GPIO Config
    DEV_GPIO_Init();
    // SPI Config
    vspi = new SPIClass(VSPI);
    vspi->begin(LCD_CLK_PIN, LCD_MISO_PIN, LCD_MOSI_PIN, LCD_CS_PIN); //SCLK, MISO, MOSI, SS
    pinMode(vspi->pinSS(), OUTPUT); //VSPI SS
    vspi->beginTransaction(SPISettings(80000000, MSBFIRST, SPI_MODE0));
    
    // I2C Config
    Wire.setPins(DEV_SDA_PIN, DEV_SCL_PIN);
    Wire.setClock(400000);
    Wire.begin();
    printf("DEV_Module_Init OK \r\n");
    return 0;
}

/******************************************************************************
function:	Module exits, closes SPI and BCM2835 library
parameter:
Info:
******************************************************************************/
void DEV_Module_Exit(void)
{
  vspi->end();
  Wire.end();
}
