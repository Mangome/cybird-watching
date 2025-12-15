/**
 * @file qmi8658_impl.h
 * @brief QMI8658 IMU 实现类
 * 
 * QMI8658 是一款6轴惯性测量单元(IMU)，集成了3轴加速度计和3轴陀螺仪
 * I2C地址：0x6A (SA0=0) 或 0x6B (SA0=1)
 */

#ifndef QMI8658_IMPL_H
#define QMI8658_IMPL_H

#include "imu_interface.h"
#include <Wire.h>

namespace HAL {

class QMI8658Impl : public IMUInterface {
public:
    explicit QMI8658Impl(uint8_t address = 0x6A);
    ~QMI8658Impl() override = default;
    
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
    const char* getTypeName() const override { return "QMI8658"; }
    uint8_t getAddress() const override { return address_; }
    
private:
    // QMI8658 寄存器地址
    static constexpr uint8_t REG_WHO_AM_I       = 0x00;  // 期望值: 0x05
    static constexpr uint8_t REG_REVISION_ID    = 0x01;
    static constexpr uint8_t REG_CTRL1          = 0x02;  // 串行接口和传感器使能
    static constexpr uint8_t REG_CTRL2          = 0x03;  // 加速度计配置
    static constexpr uint8_t REG_CTRL3          = 0x04;  // 陀螺仪配置
    static constexpr uint8_t REG_CTRL4          = 0x05;  // 传感器使能
    static constexpr uint8_t REG_CTRL5          = 0x06;  // 低通滤波器配置
    static constexpr uint8_t REG_CTRL6          = 0x07;  // 运动检测配置
    static constexpr uint8_t REG_CTRL7          = 0x08;  // 使能设置
    static constexpr uint8_t REG_CTRL8          = 0x09;  // 命令寄存器
    static constexpr uint8_t REG_CTRL9          = 0x0A;  // 主机命令寄存器
    
    static constexpr uint8_t REG_FIFO_CTRL      = 0x13;  // FIFO 控制
    static constexpr uint8_t REG_FIFO_SAMPLES   = 0x14;  // FIFO 样本数
    static constexpr uint8_t REG_FIFO_STATUS    = 0x15;  // FIFO 状态
    static constexpr uint8_t REG_FIFO_DATA      = 0x16;  // FIFO 数据
    
    static constexpr uint8_t REG_TEMP_L         = 0x33;  // 温度低字节
    static constexpr uint8_t REG_TEMP_H         = 0x34;  // 温度高字节
    
    static constexpr uint8_t REG_AX_L           = 0x35;  // 加速度X低字节
    static constexpr uint8_t REG_AX_H           = 0x36;
    static constexpr uint8_t REG_AY_L           = 0x37;
    static constexpr uint8_t REG_AY_H           = 0x38;
    static constexpr uint8_t REG_AZ_L           = 0x39;
    static constexpr uint8_t REG_AZ_H           = 0x3A;
    
    static constexpr uint8_t REG_GX_L           = 0x3B;  // 陀螺仪X低字节
    static constexpr uint8_t REG_GX_H           = 0x3C;
    static constexpr uint8_t REG_GY_L           = 0x3D;
    static constexpr uint8_t REG_GY_H           = 0x3E;
    static constexpr uint8_t REG_GZ_L           = 0x3F;
    static constexpr uint8_t REG_GZ_H           = 0x40;
    
    static constexpr uint8_t REG_RESET          = 0x60;  // 软复位
    
    // CTRL7 命令
    static constexpr uint8_t CTRL7_ENABLE_ACCEL  = 0x01;
    static constexpr uint8_t CTRL7_ENABLE_GYRO   = 0x02;
    static constexpr uint8_t CTRL7_ENABLE_MAG    = 0x04;
    static constexpr uint8_t CTRL7_ACCEL_MODE    = 0x00;  // 0=低功耗, 1=高性能
    static constexpr uint8_t CTRL7_GYRO_MODE     = 0x00;
    
    // I2C地址
    uint8_t address_;
    
    // 传感器数据
    int16_t ax_, ay_, az_;  // 加速度
    int16_t gx_, gy_, gz_;  // 陀螺仪
    
    // 校准偏移量
    int16_t accel_offset_x_, accel_offset_y_, accel_offset_z_;
    int16_t gyro_offset_x_, gyro_offset_y_, gyro_offset_z_;
    
    // 状态标志
    bool initialized_;
    unsigned long last_update_time_;
    
    // 手势检测变量（与MPU6050相同）
    unsigned long last_gesture_time_;
    int shake_counter_;
    bool was_forward_tilt_;
    bool was_backward_tilt_;
    int consecutive_tilt_count_;
    
    unsigned long last_tilt_trigger_time_;
    bool was_tilted_;
    
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
    
    // QMI8658 特定方法
    bool softReset();
    bool configureAccel();
    bool configureGyro();
    bool enableSensors();
};

} // namespace HAL

#endif // QMI8658_IMPL_H
