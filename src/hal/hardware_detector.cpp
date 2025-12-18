/**
 * @file hardware_detector.cpp
 * @brief 硬件自动检测模块实现
 */

#include "hardware_detector.h"
#include "../system/logging/log_manager.h"

namespace HAL {

// 静态成员初始化
HardwareConfig::ChipType HardwareDetector::chip_type_ = HardwareConfig::ChipType::UNKNOWN;
HardwareConfig::IMUType HardwareDetector::imu_type_ = HardwareConfig::IMUType::NONE;
HardwareConfig::SDCardMode HardwareDetector::sd_mode_ = HardwareConfig::SDCardMode::FAILED;
uint8_t HardwareDetector::imu_address_ = 0;
bool HardwareDetector::detected_ = false;

bool HardwareDetector::detect() {
    if (detected_) {
        LOG_WARN("HWDetect", "Hardware already detected, skipping...");
        return true;
    }
    
    LOG_INFO("HWDetect", "========================================");
    LOG_INFO("HWDetect", "Starting Hardware Detection...");
    LOG_INFO("HWDetect", "========================================");
    
    // 1. 检测芯片类型
    chip_type_ = detectChipType();
    LOG_INFO("HWDetect", String("Chip: ") + HardwareConfig::getPlatformName());
    
    // 2. 初始化I2C总线
    int sda = HardwareConfig::getPinIMU_SDA();
    int scl = HardwareConfig::getPinIMU_SCL();
    
    LOG_INFO("HWDetect", String("Initializing I2C: SDA=GPIO") + sda + ", SCL=GPIO" + scl);
    
    Wire.begin(sda, scl);
    Wire.setClock(HardwareConfig::getI2CFreq());
    delay(100);
    
    LOG_INFO("HWDetect", String("I2C initialized at ") + (HardwareConfig::getI2CFreq() / 1000) + " kHz");
    
    // 3. 扫描I2C总线
    int device_count = scanI2CBus(sda, scl);
    LOG_INFO("HWDetect", String("I2C scan complete: ") + device_count + " device(s) found");
    
    // 4. 检测IMU类型
    imu_type_ = detectIMUType();
    if (imu_type_ == HardwareConfig::IMUType::MPU6050) {
        LOG_INFO("HWDetect", String("IMU: MPU6050 at 0x") + String(imu_address_, HEX));
    } else if (imu_type_ == HardwareConfig::IMUType::QMI8658) {
        LOG_INFO("HWDetect", String("IMU: QMI8658 at 0x") + String(imu_address_, HEX));
    } else {
        LOG_ERROR("HWDetect", "IMU: NOT FOUND!");
    }
    
    // 5. 检测SD卡模式 (延后到SD卡初始化时)
    // 这里仅根据平台预判，实际模式在SD卡初始化时确定
#ifdef PLATFORM_ESP32_S3
    sd_mode_ = HardwareConfig::SDCardMode::SDMMC;  // ESP32-S3优先尝试SDMMC
    LOG_INFO("HWDetect", "SD Card: Will try SDMMC mode (with SPI fallback)");
#else
    sd_mode_ = HardwareConfig::SDCardMode::SPI;    // ESP32仅支持SPI
    LOG_INFO("HWDetect", "SD Card: SPI mode only");
#endif
    
    detected_ = true;
    
    LOG_INFO("HWDetect", "========================================");
    LOG_INFO("HWDetect", "Hardware Detection Complete");
    LOG_INFO("HWDetect", "========================================");
    
    return (imu_type_ != HardwareConfig::IMUType::NONE);
}

HardwareConfig::ChipType HardwareDetector::detectChipType() {
#ifdef PLATFORM_ESP32_S3
    return HardwareConfig::ChipType::ESP32_S3_CHIP;
#elif defined(PLATFORM_ESP32)
    return HardwareConfig::ChipType::ESP32_CHIP;
#else
    // 运行时检测 (备用方案)
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    
    if (chip_info.model == CHIP_ESP32S3) {
        return HardwareConfig::ChipType::ESP32_S3_CHIP;
    } else if (chip_info.model == CHIP_ESP32) {
        return HardwareConfig::ChipType::ESP32_CHIP;
    }
    
    return HardwareConfig::ChipType::UNKNOWN;
#endif
}

HardwareConfig::IMUType HardwareDetector::detectIMUType() {
    // 优先检测MPU6050
    if (identifyMPU6050()) {
        return HardwareConfig::IMUType::MPU6050;
    }
    
    // 检测QMI8658
    if (identifyQMI8658()) {
        return HardwareConfig::IMUType::QMI8658;
    }
    
    return HardwareConfig::IMUType::NONE;
}

HardwareConfig::SDCardMode HardwareDetector::detectSDCardMode() {
    // 实际SD卡模式检测在SD卡初始化时完成
    // 这里仅返回预判结果
    return sd_mode_;
}

int HardwareDetector::scanI2CBus(int sda, int scl) {
    int count = 0;
    
    LOG_INFO("HWDetect", "========================================");
    LOG_INFO("HWDetect", "Scanning I2C bus...");
    LOG_INFO("HWDetect", "========================================");
    
    for (uint8_t address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        uint8_t error = Wire.endTransmission();
        
        if (error == 0) {
            String msg = String("  [FOUND] Device at 0x") + String(address, HEX);
            if (address == 0x68) {
                msg += " (Possible MPU6050)";
            } else if (address == 0x6A || address == 0x6B) {
                msg += " (Possible QMI8658)";
            } else if (address == 0x23) {
                msg += " (Possible BH1750)";
            }
            LOG_INFO("HWDetect", msg);
            count++;
        } else if (error == 4) {
            LOG_ERROR("HWDetect", String("  [ERROR] Unknown error at 0x") + String(address, HEX));
        }
        // error == 2 表示 NACK on address (设备不存在，正常)
    }
    
    if (count == 0) {
        LOG_WARN("HWDetect", "  No I2C devices found! Check wiring.");
    }
    
    LOG_INFO("HWDetect", "========================================");
    
    return count;
}

bool HardwareDetector::checkI2CDevice(uint8_t address) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();
    return (error == 0);
}

bool HardwareDetector::readI2CRegister(uint8_t address, uint8_t reg, uint8_t* data) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    uint8_t error = Wire.endTransmission(false);
    
    if (error != 0) {
        return false;
    }
    
    uint8_t bytes = Wire.requestFrom(address, (uint8_t)1);
    if (bytes != 1) {
        return false;
    }
    
    *data = Wire.read();
    return true;
}

bool HardwareDetector::identifyMPU6050() {
    const uint8_t MPU6050_ADDR = 0x68;
    const uint8_t WHO_AM_I_REG = 0x75;
    const uint8_t EXPECTED_ID = 0x68;
    
    // 检查I2C设备是否存在
    if (!checkI2CDevice(MPU6050_ADDR)) {
        return false;
    }
    
    // 读取WHO_AM_I寄存器
    uint8_t who_am_i = 0;
    if (!readI2CRegister(MPU6050_ADDR, WHO_AM_I_REG, &who_am_i)) {
        LOG_WARN("HWDetect", "Failed to read MPU6050 WHO_AM_I register");
        return false;
    }
    
    LOG_INFO("HWDetect", String("MPU6050 WHO_AM_I: 0x") + String(who_am_i, HEX));
    
    // 验证ID
    if (who_am_i == EXPECTED_ID) {
        imu_address_ = MPU6050_ADDR;
        return true;
    }
    
    return false;
}

bool HardwareDetector::identifyQMI8658() {
    const uint8_t QMI8658_ADDR_0 = 0x6A;  // SA0=0
    const uint8_t QMI8658_ADDR_1 = 0x6B;  // SA0=1
    const uint8_t WHO_AM_I_REG = 0x00;
    const uint8_t EXPECTED_ID = 0x05;
    
    // 尝试地址 0x6A
    if (checkI2CDevice(QMI8658_ADDR_0)) {
        uint8_t who_am_i = 0;
        if (readI2CRegister(QMI8658_ADDR_0, WHO_AM_I_REG, &who_am_i)) {
            LOG_INFO("HWDetect", String("QMI8658@0x6A WHO_AM_I: 0x") + String(who_am_i, HEX));
            if (who_am_i == EXPECTED_ID) {
                imu_address_ = QMI8658_ADDR_0;
                return true;
            }
        }
    }
    
    // 尝试地址 0x6B
    if (checkI2CDevice(QMI8658_ADDR_1)) {
        uint8_t who_am_i = 0;
        if (readI2CRegister(QMI8658_ADDR_1, WHO_AM_I_REG, &who_am_i)) {
            LOG_INFO("HWDetect", String("QMI8658@0x6B WHO_AM_I: 0x") + String(who_am_i, HEX));
            if (who_am_i == EXPECTED_ID) {
                imu_address_ = QMI8658_ADDR_1;
                return true;
            }
        }
    }
    
    return false;
}

void HardwareDetector::printInfo() {
    Serial.println("========================================");
    Serial.println("Hardware Configuration");
    Serial.println("========================================");
    Serial.print("Chip: ");
    Serial.println(HardwareConfig::getPlatformName());
    
    Serial.print("IMU: ");
    if (imu_type_ == HardwareConfig::IMUType::MPU6050) {
        Serial.print("MPU6050");
    } else if (imu_type_ == HardwareConfig::IMUType::QMI8658) {
        Serial.print("QMI8658");
    } else {
        Serial.print("NONE");
    }
    Serial.print(" (0x");
    Serial.print(imu_address_, HEX);
    Serial.println(")");
    
    Serial.print("SD Card Mode: ");
    if (sd_mode_ == HardwareConfig::SDCardMode::SDMMC) {
        Serial.println("SDMMC");
    } else if (sd_mode_ == HardwareConfig::SDCardMode::SPI) {
        Serial.println("SPI");
    } else {
        Serial.println("FAILED");
    }
    
    Serial.println("========================================");
}

} // namespace HAL
