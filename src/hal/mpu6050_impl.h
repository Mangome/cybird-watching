/**
 * @file mpu6050_impl.h
 * @brief MPU6050 IMU 实现类
 * 
 * 基于原有 IMU 类的功能，包装为抽象接口实现
 */

#ifndef MPU6050_IMPL_H
#define MPU6050_IMPL_H

#include "imu_interface.h"
#include <Wire.h>

namespace HAL {

class MPU6050Impl : public IMUInterface {
public:
    explicit MPU6050Impl(uint8_t address = 0x68);
    ~MPU6050Impl() override = default;
    
    // IMUInterface 接口实现
    bool begin() override;
    void update(int interval = 10) override;
    int16_t getAccelX() override { return ax_; }
    int16_t getAccelY() override { return ay_; }
    int16_t getAccelZ() override { return az_; }
    int16_t getGyroX() override { return gx_; }
    int16_t getGyroY() override { return gy_; }
    int16_t getGyroZ() override { return gz_; }
    void calibrate() override;
    GestureType getGesture() override;
    void resetGestureState() override;
    const char* getTypeName() const override { return "MPU6050"; }
    uint8_t getAddress() const override { return address_; }
    
private:
    // I2C地址
    uint8_t address_;
    
    // 传感器数据
    int16_t ax_, ay_, az_;  // 加速度
    int16_t gx_, gy_, gz_;  // 陀螺仪
    
    // 状态标志
    bool initialized_;
    int flag_;
    unsigned long last_update_time_;
    
    // 手势检测变量
    unsigned long last_gesture_time_;
    int shake_counter_;
    bool was_forward_tilt_;
    bool was_backward_tilt_;
    int consecutive_tilt_count_;
    
    // 左右倾检测变量
    unsigned long last_tilt_trigger_time_;
    bool was_tilted_;
    
    // 持续手势检测变量
    unsigned long forward_hold_start_;
    unsigned long backward_hold_start_;
    unsigned long left_tilt_start_;
    unsigned long right_tilt_start_;
    bool forward_hold_triggered_;
    bool backward_hold_triggered_;
    
    // 手势检测辅助方法
    bool isShaking();
    bool isForwardTilt();
    bool isBackwardTilt();
    bool isLeftOrRightTilt();
    bool isLeftTilt();
    bool isRightTilt();
    
    // I2C 通信辅助方法
    bool readRegister(uint8_t reg, uint8_t* data, size_t len);
    bool writeRegister(uint8_t reg, uint8_t value);
};

} // namespace HAL

#endif // MPU6050_IMPL_H
