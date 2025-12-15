/**
 * @file imu_factory.cpp
 * @brief IMU 工厂类实现
 */

#include "imu_interface.h"
#include "mpu6050_impl.h"
#include "qmi8658_impl.h"
#include "../config/hardware_config.h"
#include "../system/logging/log_manager.h"

namespace HAL {

IMUInterface* IMUFactory::create(HardwareConfig::IMUType type, uint8_t address) {
    LOG_INFO("IMUFactory", "Creating IMU instance...");
    
    IMUInterface* imu = nullptr;
    
    switch (type) {
        case HardwareConfig::IMUType::MPU6050:
            LOG_INFO("IMUFactory", String("Type: MPU6050 (0x") + String(address, HEX) + ")");
            imu = new MPU6050Impl(address);
            break;
            
        case HardwareConfig::IMUType::QMI8658:
            LOG_INFO("IMUFactory", String("Type: QMI8658 (0x") + String(address, HEX) + ")");
            imu = new QMI8658Impl(address);
            break;
            
        case HardwareConfig::IMUType::NONE:
        default:
            LOG_ERROR("IMUFactory", "Invalid IMU type");
            return nullptr;
    }
    
    // 初始化IMU
    if (imu != nullptr) {
        if (!imu->begin()) {
            LOG_ERROR("IMUFactory", "Failed to initialize IMU");
            delete imu;
            return nullptr;
        }
        LOG_INFO("IMUFactory", "IMU created and initialized successfully");
    }
    
    return imu;
}

} // namespace HAL
