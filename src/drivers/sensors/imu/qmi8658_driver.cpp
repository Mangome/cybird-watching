/**
 * @file qmi8658_driver.cpp
 * @brief QMI8658驱动实现
 */

#include "qmi8658_driver.h"
#include "../../../system/logging/log_manager.h"

QMI8658Driver::QMI8658Driver()
    : initialized_(false)
{
}

bool QMI8658Driver::init()
{
    LOG_INFO("QMI8658", "Initializing QMI8658...");
    
    // 调用QMI8658库的初始化函数
    // QMI8658_init()会自动检测I2C地址（0x6B或0x6A）
    unsigned char result = QMI8658_init();
    
    if (result == 0) {
        LOG_ERROR("QMI8658", "Initialization failed");
        initialized_ = false;
        return false;
    }
    
    initialized_ = true;
    LOG_INFO("QMI8658", "Initialization successful");
    return true;
}

bool QMI8658Driver::readData(IMUData& data)
{
    if (!initialized_) {
        LOG_ERROR("QMI8658", "Device not initialized");
        return false;
    }
    
    // 读取原始数据（LSB值）
    short raw_acc[3] = {0};
    short raw_gyro[3] = {0};
    unsigned int tim_count = 0;
    
    QMI8658_read_xyz_raw(raw_acc, raw_gyro, &tim_count);
    
    // 保存原始值（用于手势检测）
    data.accel_x_raw = raw_acc[0];
    data.accel_y_raw = raw_acc[1];
    data.accel_z_raw = raw_acc[2];
    
    data.gyro_x_raw = raw_gyro[0];
    data.gyro_y_raw = raw_gyro[1];
    data.gyro_z_raw = raw_gyro[2];
    
    // 转换为物理单位
    // ±8g 量程: 4096 LSB/g，转换为 m/s²
    data.accel_x = (float)raw_acc[0] / 4096.0f * 9.8f;
    data.accel_y = (float)raw_acc[1] / 4096.0f * 9.8f;
    data.accel_z = (float)raw_acc[2] / 4096.0f * 9.8f;
    
    // ±512dps 量程: 64 LSB/dps
    data.gyro_x = (float)raw_gyro[0] / 64.0f;
    data.gyro_y = (float)raw_gyro[1] / 64.0f;
    data.gyro_z = (float)raw_gyro[2] / 64.0f;
    
    // QMI8658库不直接提供温度，设为0
    data.temp = 0.0f;
    
    return true;
}
