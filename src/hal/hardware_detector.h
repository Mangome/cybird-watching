/**
 * @file hardware_detector.h
 * @brief 硬件自动检测模块
 * 
 * 功能：
 * - 运行时检测芯片型号 (ESP32 / ESP32-S3)
 * - 自动识别IMU类型 (MPU6050 / QMI8658)
 * - 检测SD卡接口模式 (SDMMC / SPI)
 */

#ifndef HARDWARE_DETECTOR_H
#define HARDWARE_DETECTOR_H

#include <Arduino.h>
#include <Wire.h>
#include "../config/hardware_config.h"

namespace HAL {

class HardwareDetector {
public:
    /**
     * @brief 执行完整的硬件检测
     * @return true 检测成功，false 检测失败
     */
    static bool detect();
    
    /**
     * @brief 获取检测到的芯片类型
     */
    static HardwareConfig::ChipType getChipType() { return chip_type_; }
    
    /**
     * @brief 获取检测到的IMU类型
     */
    static HardwareConfig::IMUType getIMUType() { return imu_type_; }
    
    /**
     * @brief 获取检测到的SD卡模式
     */
    static HardwareConfig::SDCardMode getSDCardMode() { return sd_mode_; }
    
    /**
     * @brief 获取IMU的I2C地址
     */
    static uint8_t getIMUAddress() { return imu_address_; }
    
    /**
     * @brief 打印硬件信息
     */
    static void printInfo();
    
private:
    // 单例模式 - 禁止外部实例化
    HardwareDetector() = default;
    
    /**
     * @brief 检测芯片型号
     * 通过编译时宏和运行时特征识别
     */
    static HardwareConfig::ChipType detectChipType();
    
    /**
     * @brief 检测IMU类型
     * 扫描I2C总线，根据设备地址和WHO_AM_I寄存器识别
     */
    static HardwareConfig::IMUType detectIMUType();
    
    /**
     * @brief 检测SD卡接口模式
     * 优先尝试SDMMC，失败则回退到SPI
     */
    static HardwareConfig::SDCardMode detectSDCardMode();
    
    /**
     * @brief 扫描I2C总线上的设备
     * @param sda SDA引脚
     * @param scl SCL引脚
     * @return 找到的设备数量
     */
    static int scanI2CBus(int sda, int scl);
    
    /**
     * @brief 检查I2C设备是否存在
     * @param address I2C地址
     * @return true 设备存在，false 设备不存在
     */
    static bool checkI2CDevice(uint8_t address);
    
    /**
     * @brief 读取I2C寄存器
     * @param address I2C设备地址
     * @param reg 寄存器地址
     * @param data 读取的数据
     * @return true 读取成功，false 读取失败
     */
    static bool readI2CRegister(uint8_t address, uint8_t reg, uint8_t* data);
    
    /**
     * @brief 识别MPU6050
     * 检查I2C地址0x68，读取WHO_AM_I寄存器(0x75)，期望值0x68
     */
    static bool identifyMPU6050();
    
    /**
     * @brief 识别QMI8658
     * 检查I2C地址0x6A/0x6B，读取WHO_AM_I寄存器(0x00)，期望值0x05
     */
    static bool identifyQMI8658();
    
    // 检测结果
    static HardwareConfig::ChipType chip_type_;
    static HardwareConfig::IMUType imu_type_;
    static HardwareConfig::SDCardMode sd_mode_;
    static uint8_t imu_address_;
    
    // 标志位
    static bool detected_;
};

} // namespace HAL

#endif // HARDWARE_DETECTOR_H
