/**
 * @file imu_detector.h
 * @brief IMU传感器自动检测器
 * 
 * 自动检测I2C总线上的IMU传感器类型（QMI8658或MPU6050）
 */

#ifndef IMU_DETECTOR_H
#define IMU_DETECTOR_H

#include <Arduino.h>
#include <Wire.h>
#include "mpu6050_driver.h"

/**
 * @brief IMU传感器检测器
 */
class IMUDetector {
public:
    /**
     * @brief 检测并创建IMU驱动实例
     * @param sda_pin I2C SDA引脚
     * @param scl_pin I2C SCL引脚
     * @return IMUDriver指针，失败返回nullptr
     */
    static IMUDriver* detectAndCreate(int sda_pin, int scl_pin);
    
private:
    /**
     * @brief 检测I2C设备是否存在
     * @param addr I2C地址
     * @return true 存在，false 不存在
     */
    static bool probeI2CDevice(uint8_t addr);
    
    /**
     * @brief 读取I2C设备的寄存器
     * @param addr I2C地址
     * @param reg 寄存器地址
     * @param value 输出值
     * @return true 成功，false 失败
     */
    static bool readRegister(uint8_t addr, uint8_t reg, uint8_t& value);
    
    // I2C地址定义
    static constexpr uint8_t QMI8658_ADDR_0 = 0x6A;  // SA0=0
    static constexpr uint8_t QMI8658_ADDR_1 = 0x6B;  // SA0=1
    static constexpr uint8_t MPU6050_ADDR   = 0x68;
    
    // WHO_AM_I寄存器
    static constexpr uint8_t QMI8658_WHO_AM_I = 0x00;  // QMI8658的WHO_AM_I寄存器
    static constexpr uint8_t MPU6050_WHO_AM_I = 0x75;  // MPU6050的WHO_AM_I寄存器
    
    // WHO_AM_I期望值
    static constexpr uint8_t QMI8658_ID = 0x05;
    static constexpr uint8_t MPU6050_ID = 0x68;
};

#endif // IMU_DETECTOR_H
