/**
 * @file hal_manager.cpp
 * @brief 硬件抽象层管理器实现
 */

#include "hal_manager.h"
#include "imu_interface.h"
#include "../system/logging/log_manager.h"

namespace HAL {

HALManager& HALManager::getInstance() {
    static HALManager instance;
    return instance;
}

HALManager::~HALManager() {
    if (imu_ != nullptr) {
        delete imu_;
        imu_ = nullptr;
    }
}

bool HALManager::initialize() {
    if (initialized_) {
        LOG_WARN("HAL", "Already initialized");
        return true;
    }
    
    LOG_INFO("HAL", "========================================");
    LOG_INFO("HAL", "Initializing Hardware Abstraction Layer");
    LOG_INFO("HAL", "========================================");
    
    // 1. 执行硬件检测
    if (!HardwareDetector::detect()) {
        LOG_ERROR("HAL", "Hardware detection failed!");
        return false;
    }
    
    // 2. 创建 IMU 实例
    HardwareConfig::IMUType imu_type = HardwareDetector::getIMUType();
    uint8_t imu_addr = HardwareDetector::getIMUAddress();
    
    imu_ = IMUFactory::create(imu_type, imu_addr);
    if (imu_ == nullptr) {
        LOG_ERROR("HAL", "Failed to create IMU instance");
        return false;
    }
    
    LOG_INFO("HAL", String("IMU initialized: ") + imu_->getTypeName());
    
    // 3. SD卡初始化延后到实际需要时
    // SDInterface 将在调用 init() 时自动检测模式
    
    initialized_ = true;
    
    LOG_INFO("HAL", "========================================");
    LOG_INFO("HAL", "HAL Initialization Complete");
    LOG_INFO("HAL", "========================================");
    
    printHardwareInfo();
    
    return true;
}

void HALManager::printHardwareInfo() {
    Serial.println("");
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║     Hardware Configuration Info        ║");
    Serial.println("╠════════════════════════════════════════╣");
    
    // 芯片信息
    Serial.print("║ Chip:        ");
    Serial.print(HardwareConfig::getPlatformName());
    for (int i = strlen(HardwareConfig::getPlatformName()); i < 25; i++) Serial.print(" ");
    Serial.println("║");
    
    // IMU 信息
    Serial.print("║ IMU:         ");
    if (imu_ != nullptr) {
        Serial.print(imu_->getTypeName());
        Serial.print(" (0x");
        Serial.print(imu_->getAddress(), HEX);
        Serial.print(")");
        int len = strlen(imu_->getTypeName()) + 7;  // " (0xXX)"
        for (int i = len; i < 25; i++) Serial.print(" ");
    } else {
        Serial.print("NOT FOUND");
        for (int i = 9; i < 25; i++) Serial.print(" ");
    }
    Serial.println("║");
    
    // SD 卡模式
    Serial.print("║ SD Card:     ");
    HardwareConfig::SDCardMode sd_mode = HardwareDetector::getSDCardMode();
    if (sd_mode == HardwareConfig::SDCardMode::SDMMC) {
        Serial.print("SDMMC (High Speed)");
        for (int i = 18; i < 25; i++) Serial.print(" ");
    } else if (sd_mode == HardwareConfig::SDCardMode::SPI) {
        Serial.print("SPI");
        for (int i = 3; i < 25; i++) Serial.print(" ");
    } else {
        Serial.print("NOT INITIALIZED");
        for (int i = 15; i < 25; i++) Serial.print(" ");
    }
    Serial.println("║");
    
    Serial.println("╚════════════════════════════════════════╝");
    Serial.println("");
}

} // namespace HAL
