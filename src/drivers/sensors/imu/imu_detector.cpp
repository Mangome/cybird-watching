/**
 * @file imu_detector.cpp
 * @brief IMU传感器自动检测器实现
 */

#include "imu_detector.h"
#include "qmi8658_driver.h"
#include "../../../system/logging/log_manager.h"
#include <esp_task_wdt.h>

IMUDriver* IMUDetector::detectAndCreate(int sda_pin, int scl_pin)
{
    LOG_INFO("IMUDetect", "Starting IMU sensor detection...");
    LOG_INFO("IMUDetect", "I2C pins: SDA=" + String(sda_pin) + ", SCL=" + String(scl_pin));
    
    // 初始化I2C总线
    Wire.begin(sda_pin, scl_pin);
    Wire.setClock(100000);  // 降低到100kHz提高稳定性
    delay(50);  // 缩短延迟
    
    // 快速扫描：只检查已知的IMU地址（大幅减少扫描时间）
    LOG_INFO("IMUDetect", "Quick-scanning known IMU addresses...");
    uint8_t known_addrs[] = {QMI8658_ADDR_1, QMI8658_ADDR_0, MPU6050_ADDR};
    for (uint8_t addr : known_addrs) {
        if (probeI2CDevice(addr)) {
            LOG_INFO("IMUDetect", "Found device at 0x" + String(addr, HEX));
        }
    }
    
    // 按优先级检测传感器：QMI8658 > MPU6050
    
    // 喂狗，避免检测超时
    esp_task_wdt_reset();
    
    // 1. 检测QMI8658（地址0x6B）
    LOG_INFO("IMUDetect", "Checking QMI8658 at 0x6B...");
    if (probeI2CDevice(QMI8658_ADDR_1)) {
        uint8_t whoami = 0;
        if (readRegister(QMI8658_ADDR_1, QMI8658_WHO_AM_I, whoami)) {
            LOG_INFO("IMUDetect", "QMI8658 WHO_AM_I: 0x" + String(whoami, HEX));
            if (whoami == QMI8658_ID) {
                LOG_INFO("IMUDetect", "✓ QMI8658 detected at 0x6B");
                esp_task_wdt_reset();  // 喂狗
                QMI8658Driver* driver = new QMI8658Driver();
                if (driver->init()) {
                    return driver;
                } else {
                    LOG_ERROR("IMUDetect", "QMI8658 initialization failed");
                    delete driver;
                }
            }
        }
    }
    
    // 喂狗
    esp_task_wdt_reset();
    
    // 2. 检测QMI8658（地址0x6A）
    LOG_INFO("IMUDetect", "Checking QMI8658 at 0x6A...");
    if (probeI2CDevice(QMI8658_ADDR_0)) {
        uint8_t whoami = 0;
        if (readRegister(QMI8658_ADDR_0, QMI8658_WHO_AM_I, whoami)) {
            LOG_INFO("IMUDetect", "QMI8658 WHO_AM_I: 0x" + String(whoami, HEX));
            if (whoami == QMI8658_ID) {
                LOG_INFO("IMUDetect", "✓ QMI8658 detected at 0x6A");
                esp_task_wdt_reset();  // 喂狗
                QMI8658Driver* driver = new QMI8658Driver();
                if (driver->init()) {
                    return driver;
                } else {
                    LOG_ERROR("IMUDetect", "QMI8658 initialization failed");
                    delete driver;
                }
            }
        }
    }
    
    // 喂狗
    esp_task_wdt_reset();
    
    // 3. 检测MPU6050（地址0x68）
    LOG_INFO("IMUDetect", "Checking MPU6050 at 0x68...");
    if (probeI2CDevice(MPU6050_ADDR)) {
        uint8_t whoami = 0;
        if (readRegister(MPU6050_ADDR, MPU6050_WHO_AM_I, whoami)) {
            LOG_INFO("IMUDetect", "MPU6050 WHO_AM_I: 0x" + String(whoami, HEX));
            if (whoami == MPU6050_ID) {
                LOG_INFO("IMUDetect", "✓ MPU6050 detected at 0x68");
                esp_task_wdt_reset();  // 喂狗
                MPU6050Driver* driver = new MPU6050Driver(MPU6050_ADDR);
                if (driver->init()) {
                    return driver;
                } else {
                    LOG_ERROR("IMUDetect", "MPU6050 initialization failed");
                    delete driver;
                }
            }
        }
    }
    
    LOG_ERROR("IMUDetect", "No supported IMU sensor detected");
    return nullptr;
}

bool IMUDetector::probeI2CDevice(uint8_t addr)
{
    Wire.beginTransmission(addr);
    Wire.setTimeout(50);  // 设置50ms超时，避免卡死
    return Wire.endTransmission() == 0;
}

bool IMUDetector::readRegister(uint8_t addr, uint8_t reg, uint8_t& value)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }
    
    if (Wire.requestFrom(addr, (uint8_t)1) != 1) {
        return false;
    }
    
    if (Wire.available()) {
        value = Wire.read();
        return true;
    }
    
    return false;
}
