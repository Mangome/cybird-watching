/**
 * @file hal_manager.h
 * @brief 硬件抽象层管理器
 * 
 * 统一管理硬件初始化和访问
 * 为上层应用提供简单的硬件访问接口
 */

#ifndef HAL_MANAGER_H
#define HAL_MANAGER_H

#include <Arduino.h>
#include "hardware_detector.h"
#include "imu_interface.h"
#include "../config/hardware_config.h"

namespace HAL {

/**
 * @brief 硬件抽象层管理器
 * 
 * 单例类，负责：
 * 1. 硬件自动检测
 * 2. 驱动初始化
 * 3. 统一硬件访问接口
 */
class HALManager {
public:
    /**
     * @brief 获取单例实例
     */
    static HALManager& getInstance();
    
    /**
     * @brief 初始化硬件抽象层
     * @return true 初始化成功，false 初始化失败
     */
    bool initialize();
    
    /**
     * @brief 获取 IMU 接口
     * @return IMU接口指针，未初始化返回nullptr
     */
    IMUInterface* getIMU() { return imu_; }
    
    /**
     * @brief 获取检测到的硬件信息
     */
    HardwareConfig::ChipType getChipType() { return HardwareDetector::getChipType(); }
    HardwareConfig::IMUType getIMUType() { return HardwareDetector::getIMUType(); }
    HardwareConfig::SDCardMode getSDMode() { return HardwareDetector::getSDCardMode(); }
    
    /**
     * @brief 打印硬件信息
     */
    void printHardwareInfo();
    
    /**
     * @brief 检查是否已初始化
     */
    bool isInitialized() { return initialized_; }
    
private:
    // 单例模式
    HALManager() : imu_(nullptr), initialized_(false) {}
    ~HALManager();
    HALManager(const HALManager&) = delete;
    HALManager& operator=(const HALManager&) = delete;
    
    // 硬件实例
    IMUInterface* imu_;
    
    // 状态标志
    bool initialized_;
};

} // namespace HAL

#endif // HAL_MANAGER_H
