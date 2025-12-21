/**
 * @file mpu6050_driver.cpp
 * @brief MPU6050驱动实现
 */

#include "mpu6050_driver.h"
#include "../../../system/logging/log_manager.h"

MPU6050Driver::MPU6050Driver(uint8_t i2c_addr)
    : i2c_addr_(i2c_addr), initialized_(false)
{
}

bool MPU6050Driver::init()
{
    LOG_INFO("MPU6050", "Initializing MPU6050 at address 0x" + String(i2c_addr_, HEX));
    
    // 检查WHO_AM_I寄存器
    uint8_t whoami = 0;
    if (!readByte(REG_WHO_AM_I, whoami)) {
        LOG_ERROR("MPU6050", "Failed to read WHO_AM_I register");
        return false;
    }
    
    LOG_INFO("MPU6050", "WHO_AM_I: 0x" + String(whoami, HEX) + " (expected: 0x68)");
    
    if (whoami != 0x68) {
        LOG_ERROR("MPU6050", "Unexpected WHO_AM_I value");
        return false;
    }
    
    // 唤醒MPU6050（退出睡眠模式）
    if (!writeByte(REG_PWR_MGMT_1, 0x00)) {
        LOG_ERROR("MPU6050", "Failed to wake up device");
        return false;
    }
    delay(100);
    
    // 配置加速度计：±2g量程
    if (!writeByte(REG_ACCEL_CONFIG, 0x00)) {
        LOG_ERROR("MPU6050", "Failed to configure accelerometer");
        return false;
    }
    
    // 配置陀螺仪：±250°/s量程
    if (!writeByte(REG_GYRO_CONFIG, 0x00)) {
        LOG_ERROR("MPU6050", "Failed to configure gyroscope");
        return false;
    }
    
    initialized_ = true;
    LOG_INFO("MPU6050", "Initialization successful");
    return true;
}

bool MPU6050Driver::readData(IMUData& data)
{
    if (!initialized_) {
        LOG_ERROR("MPU6050", "Device not initialized");
        return false;
    }
    
    // 读取14字节：加速度(6) + 温度(2) + 陀螺仪(6)
    uint8_t buffer[14];
    if (!readBytes(REG_ACCEL_XOUT_H, buffer, 14)) {
        LOG_ERROR("MPU6050", "Failed to read sensor data");
        return false;
    }
    
    // 解析原始数据（大端序）
    data.accel_x_raw = (int16_t)((buffer[0] << 8) | buffer[1]);
    data.accel_y_raw = (int16_t)((buffer[2] << 8) | buffer[3]);
    data.accel_z_raw = (int16_t)((buffer[4] << 8) | buffer[5]);
    
    int16_t temp_raw = (int16_t)((buffer[6] << 8) | buffer[7]);
    
    data.gyro_x_raw = (int16_t)((buffer[8] << 8) | buffer[9]);
    data.gyro_y_raw = (int16_t)((buffer[10] << 8) | buffer[11]);
    data.gyro_z_raw = (int16_t)((buffer[12] << 8) | buffer[13]);
    
    // 转换为物理单位
    data.accel_x = data.accel_x_raw / ACCEL_SCALE * 9.8f;  // m/s²
    data.accel_y = data.accel_y_raw / ACCEL_SCALE * 9.8f;
    data.accel_z = data.accel_z_raw / ACCEL_SCALE * 9.8f;
    
    data.gyro_x = data.gyro_x_raw / GYRO_SCALE;  // deg/s
    data.gyro_y = data.gyro_y_raw / GYRO_SCALE;
    data.gyro_z = data.gyro_z_raw / GYRO_SCALE;
    
    data.temp = temp_raw / TEMP_SCALE + TEMP_OFFSET;  // °C
    
    return true;
}

bool MPU6050Driver::readByte(uint8_t reg, uint8_t& value)
{
    Wire.beginTransmission(i2c_addr_);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }
    
    if (Wire.requestFrom(i2c_addr_, (uint8_t)1) != 1) {
        return false;
    }
    
    if (Wire.available()) {
        value = Wire.read();
        return true;
    }
    
    return false;
}

bool MPU6050Driver::writeByte(uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(i2c_addr_);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

bool MPU6050Driver::readBytes(uint8_t reg, uint8_t* buffer, uint8_t len)
{
    Wire.beginTransmission(i2c_addr_);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }
    
    if (Wire.requestFrom(i2c_addr_, len) != len) {
        return false;
    }
    
    for (uint8_t i = 0; i < len; i++) {
        if (!Wire.available()) {
            return false;
        }
        buffer[i] = Wire.read();
    }
    
    return true;
}
