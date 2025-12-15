/**
 * @file qmi8658_impl.cpp
 * @brief QMI8658 IMU 实现类
 */

#include "qmi8658_impl.h"
#include "../system/logging/log_manager.h"

namespace HAL {

QMI8658Impl::QMI8658Impl(uint8_t address)
    : address_(address)
    , ax_(0), ay_(0), az_(0)
    , gx_(0), gy_(0), gz_(0)
    , accel_offset_x_(0), accel_offset_y_(0), accel_offset_z_(0)
    , gyro_offset_x_(0), gyro_offset_y_(0), gyro_offset_z_(0)
    , initialized_(false)
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

bool QMI8658Impl::begin() {
    LOG_INFO("QMI8658", "Initializing QMI8658...");
    
    // 读取 WHO_AM_I 寄存器
    uint8_t who_am_i = 0;
    if (!readRegister(REG_WHO_AM_I, &who_am_i, 1)) {
        LOG_ERROR("QMI8658", "Failed to read WHO_AM_I register");
        return false;
    }
    
    LOG_INFO("QMI8658", String("WHO_AM_I: 0x") + String(who_am_i, HEX) + " (expected: 0x05)");
    
    if (who_am_i != 0x05) {
        LOG_ERROR("QMI8658", "Invalid WHO_AM_I value");
        return false;
    }
    
    // 读取 Revision ID
    uint8_t revision = 0;
    if (readRegister(REG_REVISION_ID, &revision, 1)) {
        LOG_INFO("QMI8658", String("Revision ID: 0x") + String(revision, HEX));
    }
    
    // 软复位
    if (!softReset()) {
        LOG_ERROR("QMI8658", "Soft reset failed");
        return false;
    }
    
    // 配置加速度计
    if (!configureAccel()) {
        LOG_ERROR("QMI8658", "Failed to configure accelerometer");
        return false;
    }
    
    // 配置陀螺仪
    if (!configureGyro()) {
        LOG_ERROR("QMI8658", "Failed to configure gyroscope");
        return false;
    }
    
    // 使能传感器
    if (!enableSensors()) {
        LOG_ERROR("QMI8658", "Failed to enable sensors");
        return false;
    }
    
    initialized_ = true;
    resetGestureState();
    
    LOG_INFO("QMI8658", "Initialization complete!");
    return true;
}

void QMI8658Impl::update(int interval) {
    if (!initialized_) {
        return;
    }
    
    // 读取加速度计数据（从 0x35 开始，连续读取6字节）
    uint8_t accel_data[6];
    if (!readRegister(REG_AX_L, accel_data, 6)) {
        return;
    }
    
    // 解析加速度数据（小端序）
    ax_ = (int16_t)((accel_data[1] << 8) | accel_data[0]) - accel_offset_x_;
    ay_ = (int16_t)((accel_data[3] << 8) | accel_data[2]) - accel_offset_y_;
    az_ = (int16_t)((accel_data[5] << 8) | accel_data[4]) - accel_offset_z_;
    
    // 读取陀螺仪数据（从 0x3B 开始，连续读取6字节）
    uint8_t gyro_data[6];
    if (readRegister(REG_GX_L, gyro_data, 6)) {
        gx_ = (int16_t)((gyro_data[1] << 8) | gyro_data[0]) - gyro_offset_x_;
        gy_ = (int16_t)((gyro_data[3] << 8) | gyro_data[2]) - gyro_offset_y_;
        gz_ = (int16_t)((gyro_data[5] << 8) | gyro_data[4]) - gyro_offset_z_;
    }
    
    // 调试输出（每秒一次）
    static unsigned long last_debug_print = 0;
    if (millis() - last_debug_print > 1000) {
        Serial.printf("QMI8658: ax=%d, ay=%d, az=%d\n", ax_, ay_, az_);
        last_debug_print = millis();
    }
}

void QMI8658Impl::calibrate() {
    LOG_INFO("QMI8658", "Starting calibration...");
    LOG_INFO("QMI8658", "Please keep the device STILL on a flat surface!");
    
    delay(2000);  // 等待2秒让用户放置设备
    
    // 采样100次求平均值
    const int samples = 100;
    int32_t sum_ax = 0, sum_ay = 0, sum_az = 0;
    int32_t sum_gx = 0, sum_gy = 0, sum_gz = 0;
    
    for (int i = 0; i < samples; i++) {
        // 读取加速度计
        uint8_t accel_data[6];
        if (readRegister(REG_AX_L, accel_data, 6)) {
            sum_ax += (int16_t)((accel_data[1] << 8) | accel_data[0]);
            sum_ay += (int16_t)((accel_data[3] << 8) | accel_data[2]);
            sum_az += (int16_t)((accel_data[5] << 8) | accel_data[4]);
        }
        
        // 读取陀螺仪
        uint8_t gyro_data[6];
        if (readRegister(REG_GX_L, gyro_data, 6)) {
            sum_gx += (int16_t)((gyro_data[1] << 8) | gyro_data[0]);
            sum_gy += (int16_t)((gyro_data[3] << 8) | gyro_data[2]);
            sum_gz += (int16_t)((gyro_data[5] << 8) | gyro_data[4]);
        }
        
        delay(10);
    }
    
    // 计算平均值作为偏移量
    accel_offset_x_ = sum_ax / samples;
    accel_offset_y_ = sum_ay / samples;
    accel_offset_z_ = (sum_az / samples) - 16384;  // Z轴应该读取1g (16384 LSB @ ±2g)
    
    gyro_offset_x_ = sum_gx / samples;
    gyro_offset_y_ = sum_gy / samples;
    gyro_offset_z_ = sum_gz / samples;
    
    LOG_INFO("QMI8658", String("Accel offsets: X=") + accel_offset_x_ + ", Y=" + accel_offset_y_ + ", Z=" + accel_offset_z_);
    LOG_INFO("QMI8658", String("Gyro offsets: X=") + gyro_offset_x_ + ", Y=" + gyro_offset_y_ + ", Z=" + gyro_offset_z_);
    LOG_INFO("QMI8658", "Calibration complete!");
}

GestureType QMI8658Impl::getGesture() {
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
            LOG_INFO("QMI8658", "Gesture: FORWARD_HOLD (1s)");
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
            LOG_INFO("QMI8658", "Gesture: BACKWARD_HOLD (1s)");
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
            LOG_INFO("QMI8658", "Gesture: LEFT_TILT");
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
            LOG_INFO("QMI8658", "Gesture: RIGHT_TILT");
            return GestureType::RIGHT_TILT;
        }
    } else {
        right_tilt_start_ = 0;
    }
    
    // 检测摇动手势
    if (isShaking()) {
        LOG_INFO("QMI8658", "Gesture: SHAKE");
        return GestureType::SHAKE;
    }
    
    return GestureType::NONE;
}

void QMI8658Impl::resetGestureState() {
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

bool QMI8658Impl::isShaking() {
    static int16_t last_ax = 0, last_ay = 0, last_az = 0;
    
    int16_t delta_ax = abs(ax_ - last_ax);
    int16_t delta_ay = abs(ay_ - last_ay);
    int16_t delta_az = abs(az_ - last_az);
    
    last_ax = ax_;
    last_ay = ay_;
    last_az = az_;
    
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

bool QMI8658Impl::isForwardTilt() {
    return (ax_ < -10000);
}

bool QMI8658Impl::isBackwardTilt() {
    return (ax_ > 14000);
}

bool QMI8658Impl::isLeftOrRightTilt() {
    return (ay_ > 10000 || ay_ < -10000);
}

bool QMI8658Impl::isLeftTilt() {
    return (ay_ > 10000);
}

bool QMI8658Impl::isRightTilt() {
    return (ay_ < -10000);
}

bool QMI8658Impl::readRegister(uint8_t reg, uint8_t* data, size_t len) {
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

bool QMI8658Impl::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(address_);
    Wire.write(reg);
    Wire.write(value);
    uint8_t error = Wire.endTransmission();
    
    return (error == 0);
}

bool QMI8658Impl::softReset() {
    LOG_INFO("QMI8658", "Performing soft reset...");
    
    // 写入复位命令
    if (!writeRegister(REG_RESET, 0xB0)) {
        return false;
    }
    
    delay(10);  // 等待复位完成
    
    // 检查WHO_AM_I确认复位成功
    uint8_t who_am_i = 0;
    if (!readRegister(REG_WHO_AM_I, &who_am_i, 1)) {
        return false;
    }
    
    return (who_am_i == 0x05);
}

bool QMI8658Impl::configureAccel() {
    LOG_INFO("QMI8658", "Configuring accelerometer...");
    
    // CTRL2: 加速度计配置
    // [7:4] = ODR (1000 = 1000Hz)
    // [3:1] = Full Scale (000 = ±2g)
    // [0]   = 保留
    uint8_t ctrl2 = 0x80;  // 1000Hz, ±2g
    
    return writeRegister(REG_CTRL2, ctrl2);
}

bool QMI8658Impl::configureGyro() {
    LOG_INFO("QMI8658", "Configuring gyroscope...");
    
    // CTRL3: 陀螺仪配置
    // [7:4] = ODR (1000 = 1000Hz)
    // [3:1] = Full Scale (001 = ±512dps)
    // [0]   = 保留
    uint8_t ctrl3 = 0x82;  // 1000Hz, ±512dps
    
    return writeRegister(REG_CTRL3, ctrl3);
}

bool QMI8658Impl::enableSensors() {
    LOG_INFO("QMI8658", "Enabling sensors...");
    
    // CTRL7: 使能加速度计和陀螺仪
    uint8_t ctrl7 = CTRL7_ENABLE_ACCEL | CTRL7_ENABLE_GYRO;
    
    return writeRegister(REG_CTRL7, ctrl7);
}

} // namespace HAL
