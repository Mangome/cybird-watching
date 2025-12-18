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
    
    // QMI8658库提供的数据缓冲区
    float acc[3] = {0};   // 加速度 (mg或m/s²)
    float gyro[3] = {0};  // 角速度 (dps或rad/s)
    unsigned int tim_count = 0;
    
    // 读取传感器数据
    QMI8658_read_xyz(acc, gyro, &tim_count);
    
    // 转换为统一的IMUData格式
    // QMI8658_read_xyz返回的单位需要根据库配置确定
    // 参考代码中使用的是mg和dps，需要转换为m/s²和deg/s
    data.accel_x = acc[0] / 1000.0f * 9.8f;  // mg -> m/s²
    data.accel_y = acc[1] / 1000.0f * 9.8f;
    data.accel_z = acc[2] / 1000.0f * 9.8f;
    
    data.gyro_x = gyro[0];  // dps（已经是正确单位）
    data.gyro_y = gyro[1];
    data.gyro_z = gyro[2];
    
    // QMI8658库不直接提供温度，设为0
    data.temp = 0.0f;
    
    // 计算原始值（用于手势检测等需要原始值的场景）
    // 根据±8g量程计算
    data.accel_x_raw = (int16_t)(acc[0] / ACCEL_SCALE);
    data.accel_y_raw = (int16_t)(acc[1] / ACCEL_SCALE);
    data.accel_z_raw = (int16_t)(acc[2] / ACCEL_SCALE);
    
    // 根据±512dps量程计算
    data.gyro_x_raw = (int16_t)(gyro[0] / GYRO_SCALE);
    data.gyro_y_raw = (int16_t)(gyro[1] / GYRO_SCALE);
    data.gyro_z_raw = (int16_t)(gyro[2] / GYRO_SCALE);
    
    return true;
}
