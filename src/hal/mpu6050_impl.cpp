/**
 * @file mpu6050_impl.cpp
 * @brief MPU6050 IMU 实现类
 */

#include "mpu6050_impl.h"
#include "../system/logging/log_manager.h"

namespace HAL {

MPU6050Impl::MPU6050Impl(uint8_t address)
    : address_(address)
    , ax_(0), ay_(0), az_(0)
    , gx_(0), gy_(0), gz_(0)
    , initialized_(false)
    , flag_(1)
    , last_update_time_(0)
    , last_gesture_time_(0)
    , shake_counter_(0)
    , was_forward_tilt_(false)
    , was_backward_tilt_(false)
    , consecutive_tilt_count_(0)
    , last_tilt_trigger_time_(0)
    , was_tilted_(false)
    , forward_hold_start_(0)
    , backward_hold_start_(0)
    , left_tilt_start_(0)
    , right_tilt_start_(0)
    , forward_hold_triggered_(false)
    , backward_hold_triggered_(false)
{
}

bool MPU6050Impl::begin() {
    LOG_INFO("MPU6050", "Initializing MPU6050...");
    
    // 尝试读取 WHO_AM_I 寄存器
    uint8_t who_am_i = 0;
    if (!readRegister(0x75, &who_am_i, 1)) {
        LOG_ERROR("MPU6050", "Failed to read WHO_AM_I register");
        return false;
    }
    
    LOG_INFO("MPU6050", String("WHO_AM_I: 0x") + String(who_am_i, HEX) + " (expected: 0x68)");
    
    if (who_am_i != 0x68) {
        LOG_ERROR("MPU6050", "Invalid WHO_AM_I value");
        return false;
    }
    
    // 唤醒 MPU6050
    LOG_INFO("MPU6050", "Waking up MPU6050...");
    if (!writeRegister(0x6B, 0x00)) {  // PWR_MGMT_1 寄存器，清除睡眠位
        LOG_ERROR("MPU6050", "Failed to wake up");
        return false;
    }
    delay(100);  // 等待稳定
    
    // 配置加速度计量程（±2g）
    LOG_INFO("MPU6050", "Configuring accelerometer (±2g)...");
    if (!writeRegister(0x1C, 0x00)) {  // ACCEL_CONFIG 寄存器
        LOG_ERROR("MPU6050", "Failed to configure accelerometer");
        return false;
    }
    
    // 配置陀螺仪量程（±250°/s）
    LOG_INFO("MPU6050", "Configuring gyroscope (±250°/s)...");
    if (!writeRegister(0x1B, 0x00)) {  // GYRO_CONFIG 寄存器
        LOG_ERROR("MPU6050", "Failed to configure gyroscope");
        return false;
    }
    
    initialized_ = true;
    resetGestureState();
    
    LOG_INFO("MPU6050", "Initialization complete!");
    return true;
}

void MPU6050Impl::update(int interval) {
    if (!initialized_) {
        return;
    }
    
    // 读取加速度计数据（寄存器 0x3B-0x40）
    uint8_t accel_data[6];
    if (!readRegister(0x3B, accel_data, 6)) {
        return;
    }
    
    // 解析加速度数据
    ax_ = (int16_t)((accel_data[0] << 8) | accel_data[1]);
    ay_ = (int16_t)((accel_data[2] << 8) | accel_data[3]);
    az_ = (int16_t)((accel_data[4] << 8) | accel_data[5]);
    
    // 读取陀螺仪数据（寄存器 0x43-0x48）
    uint8_t gyro_data[6];
    if (readRegister(0x43, gyro_data, 6)) {
        gx_ = (int16_t)((gyro_data[0] << 8) | gyro_data[1]);
        gy_ = (int16_t)((gyro_data[2] << 8) | gyro_data[3]);
        gz_ = (int16_t)((gyro_data[4] << 8) | gyro_data[5]);
    }
    
    // 调试输出（每秒一次）
    static unsigned long last_debug_print = 0;
    if (millis() - last_debug_print > 1000) {
        Serial.printf("MPU6050: ax=%d, ay=%d, az=%d\n", ax_, ay_, az_);
        last_debug_print = millis();
    }
}

void MPU6050Impl::calibrate() {
    // MPU6050 校准功能当前未实现
    // 可以在这里添加零偏校准逻辑
    LOG_WARN("MPU6050", "Calibration not implemented (using fixed thresholds)");
}

GestureType MPU6050Impl::getGesture() {
    if (!initialized_) {
        return GestureType::NONE;
    }
    
    unsigned long current_time = millis();
    
    // 检测持续前倾手势（保持1秒）
    if (isForwardTilt()) {
        if (forward_hold_start_ == 0) {
            forward_hold_start_ = current_time;
            forward_hold_triggered_ = false;
        } else if (!forward_hold_triggered_ && (current_time - forward_hold_start_ >= 1000)) {
            forward_hold_triggered_ = true;
            LOG_INFO("MPU6050", "Gesture: FORWARD_HOLD (1s)");
            return GestureType::FORWARD_HOLD;
        }
    } else {
        forward_hold_start_ = 0;
        forward_hold_triggered_ = false;
    }
    
    // 检测持续后倾手势（保持1秒）
    if (isBackwardTilt()) {
        if (backward_hold_start_ == 0) {
            backward_hold_start_ = current_time;
            backward_hold_triggered_ = false;
        } else if (!backward_hold_triggered_ && (current_time - backward_hold_start_ >= 1000)) {
            backward_hold_triggered_ = true;
            LOG_INFO("MPU6050", "Gesture: BACKWARD_HOLD (1s)");
            return GestureType::BACKWARD_HOLD;
        }
    } else {
        backward_hold_start_ = 0;
        backward_hold_triggered_ = false;
    }
    
    // 检测左倾手势（0.5秒）
    if (isLeftTilt()) {
        if (left_tilt_start_ == 0) {
            left_tilt_start_ = current_time;
        } else if (current_time - left_tilt_start_ >= 500) {
            left_tilt_start_ = 0;
            LOG_INFO("MPU6050", "Gesture: LEFT_TILT");
            return GestureType::LEFT_TILT;
        }
    } else {
        left_tilt_start_ = 0;
    }
    
    // 检测右倾手势（0.5秒）
    if (isRightTilt()) {
        if (right_tilt_start_ == 0) {
            right_tilt_start_ = current_time;
        } else if (current_time - right_tilt_start_ >= 500) {
            right_tilt_start_ = 0;
            LOG_INFO("MPU6050", "Gesture: RIGHT_TILT");
            return GestureType::RIGHT_TILT;
        }
    } else {
        right_tilt_start_ = 0;
    }
    
    // 检测摇动手势
    if (isShaking()) {
        LOG_INFO("MPU6050", "Gesture: SHAKE");
        return GestureType::SHAKE;
    }
    
    return GestureType::NONE;
}

void MPU6050Impl::resetGestureState() {
    last_gesture_time_ = 0;
    shake_counter_ = 0;
    was_forward_tilt_ = false;
    was_backward_tilt_ = false;
    consecutive_tilt_count_ = 0;
    
    last_tilt_trigger_time_ = 0;
    was_tilted_ = false;
    
    forward_hold_start_ = 0;
    backward_hold_start_ = 0;
    left_tilt_start_ = 0;
    right_tilt_start_ = 0;
    forward_hold_triggered_ = false;
    backward_hold_triggered_ = false;
}

// ==================== 私有方法 ====================

bool MPU6050Impl::isShaking() {
    static int16_t last_ax = 0, last_ay = 0, last_az = 0;
    
    int16_t delta_ax = abs(ax_ - last_ax);
    int16_t delta_ay = abs(ay_ - last_ay);
    int16_t delta_az = abs(az_ - last_az);
    
    last_ax = ax_;
    last_ay = ay_;
    last_az = az_;
    
    // 如果加速度变化很大，认为是摇动
    if (delta_ax > 8000 || delta_ay > 8000 || delta_az > 8000) {
        shake_counter_++;
        if (shake_counter_ > 3) {
            shake_counter_ = 0;
            return true;
        }
    } else {
        shake_counter_ = 0;
    }
    
    return false;
}

bool MPU6050Impl::isForwardTilt() {
    // 前倾：X轴负值较大
    return (ax_ < -10000);
}

bool MPU6050Impl::isBackwardTilt() {
    // 后倾：X轴正值较大
    return (ax_ > 14000);
}

bool MPU6050Impl::isLeftOrRightTilt() {
    // Y轴绝对值大于10000
    return (ay_ > 10000 || ay_ < -10000);
}

bool MPU6050Impl::isLeftTilt() {
    // 左倾：Y轴正值大于10000
    return (ay_ > 10000);
}

bool MPU6050Impl::isRightTilt() {
    // 右倾：Y轴负值小于-10000
    return (ay_ < -10000);
}

bool MPU6050Impl::readRegister(uint8_t reg, uint8_t* data, size_t len) {
    Wire.beginTransmission(address_);
    Wire.write(reg);
    uint8_t error = Wire.endTransmission(false);
    
    if (error != 0) {
        return false;
    }
    
    size_t bytes = Wire.requestFrom(address_, len);
    if (bytes != len) {
        return false;
    }
    
    for (size_t i = 0; i < len; i++) {
        data[i] = Wire.read();
    }
    
    return true;
}

bool MPU6050Impl::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(address_);
    Wire.write(reg);
    Wire.write(value);
    uint8_t error = Wire.endTransmission();
    
    return (error == 0);
}

} // namespace HAL
