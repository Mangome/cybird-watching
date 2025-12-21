/**
 * @file qmi8658_driver.h
 * @brief QMI8658 IMU传感器驱动封装
 * 
 * 基于QMI8658库实现的驱动封装，提供与MPU6050统一的接口
 */

#ifndef QMI8658_DRIVER_H
#define QMI8658_DRIVER_H

#include "mpu6050_driver.h"  // 使用统一的IMUDriver接口
#include <QMI8658.h>

/**
 * @brief QMI8658驱动类
 */
class QMI8658Driver : public IMUDriver {
public:
    /**
     * @brief 构造函数
     */
    QMI8658Driver();
    
    /**
     * @brief 初始化QMI8658
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
     * @return IMUSensorType::QMI8658
     */
    IMUSensorType getType() override { return IMUSensorType::QMI8658; }
    
    /**
     * @brief 获取手势检测阈值（±8g量程，4096 LSB/g）
     * @return 阈值结构
     */
    IMUGestureThresholds getGestureThresholds() override {
        return {
            2000,  // shake: ~0.5g
            -2500, // forward_tilt: ~-0.6g
            3500,  // backward_tilt: ~0.85g
            2500,  // left_tilt: ~0.6g
            -2500  // right_tilt: ~-0.6g
        };
    }
    
private:
    bool initialized_;      ///< 初始化标志
    
    // QMI8658配置参数（参考参考代码）
    static constexpr float ACCEL_SCALE = 8000.0f / 32768.0f;   ///< ±8g量程
    static constexpr float GYRO_SCALE = 512.0f / 32768.0f;     ///< ±512dps量程
};

#endif // QMI8658_DRIVER_H
