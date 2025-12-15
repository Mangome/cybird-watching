/**
 * @file hardware_config.h
 * @brief 硬件平台配置和引脚映射
 * 
 * 支持多平台硬件配置：
 * - ESP32 (pico32)
 * - ESP32-S3
 * 
 * 自动根据编译目标选择对应的引脚配置
 */

#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#include <Arduino.h>

// ==================== 平台识别 ====================
// 注意：避免重复定义，platformio.ini 会通过 -D 参数定义这些宏
#if defined(CONFIG_IDF_TARGET_ESP32S3) && !defined(PLATFORM_ESP32_S3)
    #define PLATFORM_ESP32_S3
#endif

#if defined(CONFIG_IDF_TARGET_ESP32) && !defined(PLATFORM_ESP32)
    #define PLATFORM_ESP32
#endif

#if defined(PLATFORM_ESP32_S3)
    #define PLATFORM_NAME "ESP32-S3"
#elif defined(PLATFORM_ESP32)
    #define PLATFORM_NAME "ESP32"
#else
    #error "Unsupported platform! Only ESP32 and ESP32-S3 are supported."
#endif

// ==================== 硬件版本配置 ====================
namespace HardwareConfig {

// 硬件版本枚举（避免与 Arduino 框架宏冲突）
enum class ChipType {
    ESP32_CHIP,      // ESP32 芯片
    ESP32_S3_CHIP,   // ESP32-S3 芯片
    UNKNOWN
};

enum class IMUType {
    MPU6050,    // I2C地址: 0x68
    QMI8658,    // I2C地址: 0x6A 或 0x6B
    NONE
};

enum class SDCardMode {
    SPI,        // SPI模式（兼容模式）
    SDMMC,      // SDMMC模式（高速模式，仅ESP32-S3部分型号支持）
    FAILED
};

// ==================== ESP32 引脚配置 ====================
#ifdef PLATFORM_ESP32

namespace ESP32Pins {
    // TFT 显示 (VSPI - 默认SPI总线)
    constexpr int TFT_MISO      = 19;
    constexpr int TFT_MOSI      = 23;
    constexpr int TFT_SCLK      = 18;
    constexpr int TFT_CS        = -1;   // 不使用片选
    constexpr int TFT_DC        = 2;    // 数据/命令选择
    constexpr int TFT_RST       = 4;    // 复位
    constexpr int TFT_BL        = 5;    // 背光
    
    // SD卡 (HSPI - 独立SPI总线)
    constexpr int SD_SCK        = 14;
    constexpr int SD_MISO       = 26;   // 使用GPIO26避开GPIO12启动问题
    constexpr int SD_MOSI       = 13;
    constexpr int SD_CS         = 15;
    
    // IMU MPU6050 (I2C)
    constexpr int IMU_SDA       = 32;
    constexpr int IMU_SCL       = 33;
    constexpr uint8_t IMU_I2C_ADDR = 0x68;
    
    // 环境光传感器 BH1750 (I2C - 共享总线)
    constexpr int AMB_SDA       = 32;
    constexpr int AMB_SCL       = 33;
    constexpr uint8_t AMB_I2C_ADDR = 0x23;
    
    // RGB LED (WS2812)
    constexpr int RGB_LED_PIN   = 27;
    constexpr int RGB_LED_NUM   = 2;
    
    // I2C 配置
    constexpr uint32_t I2C_FREQUENCY = 100000;  // 100kHz
    
    // SPI 配置
    constexpr uint32_t SD_SPI_FREQUENCY = 25000000;  // 25MHz (最大)
}

#endif // PLATFORM_ESP32

// ==================== ESP32-S3 引脚配置 ====================
#ifdef PLATFORM_ESP32_S3

namespace ESP32S3Pins {
    // TFT 显示 (SPI2 - 默认SPI总线)
    constexpr int TFT_MISO      = 13;   // ESP32-S3 SPI2默认引脚
    constexpr int TFT_MOSI      = 11;
    constexpr int TFT_SCLK      = 12;
    constexpr int TFT_CS        = -1;   // 不使用片选
    constexpr int TFT_DC        = 10;   // 数据/命令选择
    constexpr int TFT_RST       = 9;    // 复位
    constexpr int TFT_BL        = 8;    // 背光
    
    // SD卡 SDMMC 模式（优先）
    constexpr int SDMMC_CLK     = 36;
    constexpr int SDMMC_CMD     = 35;
    constexpr int SDMMC_D0      = 37;
    constexpr int SDMMC_D1      = 38;
    constexpr int SDMMC_D2      = 39;
    constexpr int SDMMC_D3      = 40;
    
    // SD卡 SPI 模式（回退方案）
    constexpr int SD_SCK        = 14;
    constexpr int SD_MISO       = 15;
    constexpr int SD_MOSI       = 16;
    constexpr int SD_CS         = 17;
    
    // IMU (I2C - 兼容 MPU6050 和 QMI8658)
    constexpr int IMU_SDA       = 1;
    constexpr int IMU_SCL       = 2;
    // I2C地址在运行时检测
    constexpr uint8_t MPU6050_I2C_ADDR = 0x68;
    constexpr uint8_t QMI8658_I2C_ADDR_0 = 0x6A;  // SA0=0
    constexpr uint8_t QMI8658_I2C_ADDR_1 = 0x6B;  // SA0=1
    
    // 环境光传感器 BH1750 (I2C - 共享总线)
    constexpr int AMB_SDA       = 1;
    constexpr int AMB_SCL       = 2;
    constexpr uint8_t AMB_I2C_ADDR = 0x23;
    
    // RGB LED (WS2812)
    constexpr int RGB_LED_PIN   = 48;   // ESP32-S3 推荐GPIO48
    constexpr int RGB_LED_NUM   = 2;
    
    // I2C 配置
    constexpr uint32_t I2C_FREQUENCY = 100000;  // 100kHz
    
    // SDMMC 配置
    constexpr uint32_t SDMMC_FREQUENCY = 40000000;  // 40MHz (高速模式)
    
    // SPI 配置
    constexpr uint32_t SD_SPI_FREQUENCY = 25000000;  // 25MHz
}

#endif // PLATFORM_ESP32_S3

// ==================== 统一引脚访问接口 ====================
/**
 * @brief 获取当前平台的引脚配置
 * 
 * 使用方法：
 * int sda_pin = HardwareConfig::getPinIMU_SDA();
 */

#ifdef PLATFORM_ESP32
    inline int getPinTFT_MISO()     { return ESP32Pins::TFT_MISO; }
    inline int getPinTFT_MOSI()     { return ESP32Pins::TFT_MOSI; }
    inline int getPinTFT_SCLK()     { return ESP32Pins::TFT_SCLK; }
    inline int getPinTFT_CS()       { return ESP32Pins::TFT_CS; }
    inline int getPinTFT_DC()       { return ESP32Pins::TFT_DC; }
    inline int getPinTFT_RST()      { return ESP32Pins::TFT_RST; }
    inline int getPinTFT_BL()       { return ESP32Pins::TFT_BL; }
    
    inline int getPinSD_SCK()       { return ESP32Pins::SD_SCK; }
    inline int getPinSD_MISO()      { return ESP32Pins::SD_MISO; }
    inline int getPinSD_MOSI()      { return ESP32Pins::SD_MOSI; }
    inline int getPinSD_CS()        { return ESP32Pins::SD_CS; }
    
    inline int getPinIMU_SDA()      { return ESP32Pins::IMU_SDA; }
    inline int getPinIMU_SCL()      { return ESP32Pins::IMU_SCL; }
    
    inline int getPinRGB_LED()      { return ESP32Pins::RGB_LED_PIN; }
    inline int getRGB_LED_NUM()     { return ESP32Pins::RGB_LED_NUM; }
    
    inline uint32_t getI2CFreq()    { return ESP32Pins::I2C_FREQUENCY; }
    inline uint32_t getSDSPIFreq()  { return ESP32Pins::SD_SPI_FREQUENCY; }
#endif

#ifdef PLATFORM_ESP32_S3
    inline int getPinTFT_MISO()     { return ESP32S3Pins::TFT_MISO; }
    inline int getPinTFT_MOSI()     { return ESP32S3Pins::TFT_MOSI; }
    inline int getPinTFT_SCLK()     { return ESP32S3Pins::TFT_SCLK; }
    inline int getPinTFT_CS()       { return ESP32S3Pins::TFT_CS; }
    inline int getPinTFT_DC()       { return ESP32S3Pins::TFT_DC; }
    inline int getPinTFT_RST()      { return ESP32S3Pins::TFT_RST; }
    inline int getPinTFT_BL()       { return ESP32S3Pins::TFT_BL; }
    
    inline int getPinSD_SCK()       { return ESP32S3Pins::SD_SCK; }
    inline int getPinSD_MISO()      { return ESP32S3Pins::SD_MISO; }
    inline int getPinSD_MOSI()      { return ESP32S3Pins::SD_MOSI; }
    inline int getPinSD_CS()        { return ESP32S3Pins::SD_CS; }
    
    inline int getPinSDMMC_CLK()    { return ESP32S3Pins::SDMMC_CLK; }
    inline int getPinSDMMC_CMD()    { return ESP32S3Pins::SDMMC_CMD; }
    inline int getPinSDMMC_D0()     { return ESP32S3Pins::SDMMC_D0; }
    inline int getPinSDMMC_D1()     { return ESP32S3Pins::SDMMC_D1; }
    inline int getPinSDMMC_D2()     { return ESP32S3Pins::SDMMC_D2; }
    inline int getPinSDMMC_D3()     { return ESP32S3Pins::SDMMC_D3; }
    
    inline int getPinIMU_SDA()      { return ESP32S3Pins::IMU_SDA; }
    inline int getPinIMU_SCL()      { return ESP32S3Pins::IMU_SCL; }
    
    inline int getPinRGB_LED()      { return ESP32S3Pins::RGB_LED_PIN; }
    inline int getRGB_LED_NUM()     { return ESP32S3Pins::RGB_LED_NUM; }
    
    inline uint32_t getI2CFreq()    { return ESP32S3Pins::I2C_FREQUENCY; }
    inline uint32_t getSDSPIFreq()  { return ESP32S3Pins::SD_SPI_FREQUENCY; }
    inline uint32_t getSDMMCFreq()  { return ESP32S3Pins::SDMMC_FREQUENCY; }
#endif

/**
 * @brief 获取当前芯片类型
 */
inline ChipType getChipType() {
#ifdef PLATFORM_ESP32
    return ChipType::ESP32_CHIP;
#elif defined(PLATFORM_ESP32_S3)
    return ChipType::ESP32_S3_CHIP;
#else
    return ChipType::UNKNOWN;
#endif
}

/**
 * @brief 获取平台名称
 */
inline const char* getPlatformName() {
    return PLATFORM_NAME;
}

} // namespace HardwareConfig

#endif // HARDWARE_CONFIG_H
