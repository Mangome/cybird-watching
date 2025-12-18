/**
 * @file mpu6050_driver.h
 * @brief MPU6050 IMU传感器驱动封装
 * 
 * 提供MPU6050传感器的初始化和数据读取接口
 */

#ifndef MPU6050_DRIVER_H
#define MPU6050_DRIVER_H

#include <Arduino.h>
#include <Wire.h>

/**
 * @brief IMU传感器类型枚举
 */
enum class IMUSensorType {
    NONE,
    MPU6050,
    QMI8658
};

/**
 * @brief IMU数据结构
 */
struct IMUData {
    float accel_x;     ///< X轴加速度 (m/s²)
    float accel_y;     ///< Y轴加速度 (m/s²)
    float accel_z;     ///< Z轴加速度 (m/s²)
    float gyro_x;      ///< X轴角速度 (deg/s)
    float gyro_y;      ///< Y轴角速度 (deg/s)
    float gyro_z;      ///< Z轴角速度 (deg/s)
    float temp;        ///< 温度 (°C)
    
    // 原始数据（用于手势检测等需要原始值的场景）
    int16_t accel_x_raw;
    int16_t accel_y_raw;
    int16_t accel_z_raw;
    int16_t gyro_x_raw;
    int16_t gyro_y_raw;
    int16_t gyro_z_raw;
};

/**
 * @brief IMU驱动抽象接口
 */
class IMUDriver {
public:
    virtual ~IMUDriver() = default;
    
    /**
     * @brief 初始化传感器
     * @return true 初始化成功，false 失败
     */
    virtual bool init() = 0;
    
    /**
     * @brief 读取传感器数据
     * @param data 数据输出结构
     * @return true 读取成功，false 失败
     */
    virtual bool readData(IMUData& data) = 0;
    
    /**
     * @brief 获取传感器类型
     * @return 传感器类型枚举
     */
    virtual IMUSensorType getType() = 0;
};

/**
 * @brief MPU6050驱动类
 */
class MPU6050Driver : public IMUDriver {
public:
    /**
     * @brief 构造函数
     * @param i2c_addr I2C地址（默认0x68）
     */
    MPU6050Driver(uint8_t i2c_addr = 0x68);
    
    /**
     * @brief 初始化MPU6050
     * @return true 成功，false 失败
     */
    bool init() override;
    
    /**
     * @brief 读取传感器数据
     * @param data 数据输出结构
     * @return true 成功，false 失败
     */
    bool readData(IMUData& data) override;
    
    /**
     * @brief 获取传感器类型
     * @return IMUSensorType::MPU6050
     */
    IMUSensorType getType() override { return IMUSensorType::MPU6050; }
    
private:
    uint8_t i2c_addr_;      ///< I2C地址
    bool initialized_;      ///< 初始化标志
    
    // MPU6050寄存器地址
    static constexpr uint8_t REG_WHO_AM_I    = 0x75;
    static constexpr uint8_t REG_PWR_MGMT_1  = 0x6B;
    static constexpr uint8_t REG_ACCEL_CONFIG = 0x1C;
    static constexpr uint8_t REG_GYRO_CONFIG = 0x1B;
    static constexpr uint8_t REG_ACCEL_XOUT_H = 0x3B;
    static constexpr uint8_t REG_TEMP_OUT_H  = 0x41;
    
    // 加速度和陀螺仪量程
    static constexpr float ACCEL_SCALE = 16384.0f;  ///< ±2g量程
    static constexpr float GYRO_SCALE = 131.0f;     ///< ±250°/s量程
    static constexpr float TEMP_OFFSET = 36.53f;    ///< 温度偏移
    static constexpr float TEMP_SCALE = 340.0f;     ///< 温度比例
    
    /**
     * @brief 读取单个字节
     * @param reg 寄存器地址
     * @param value 输出值
     * @return true 成功，false 失败
     */
    bool readByte(uint8_t reg, uint8_t& value);
    
    /**
     * @brief 写入单个字节
     * @param reg 寄存器地址
     * @param value 写入值
     * @return true 成功，false 失败
     */
    bool writeByte(uint8_t reg, uint8_t value);
    
    /**
     * @brief 读取多个字节
     * @param reg 起始寄存器地址
     * @param buffer 数据缓冲区
     * @param len 读取长度
     * @return true 成功，false 失败
     */
    bool readBytes(uint8_t reg, uint8_t* buffer, uint8_t len);
};

#endif // MPU6050_DRIVER_H
