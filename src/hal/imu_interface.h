/**
 * @file imu_interface.h
 * @brief IMU 抽象接口
 * 
 * 统一 MPU6050 和 QMI8658 的访问接口
 * 支持运行时多态，便于扩展新的IMU类型
 */

#ifndef IMU_INTERFACE_H
#define IMU_INTERFACE_H

#include <Arduino.h>

// 前向声明 HardwareConfig 命名空间的类型
namespace HardwareConfig {
    enum class IMUType;
}

namespace HAL {

/**
 * @brief 手势类型枚举
 * 保持与原IMU类相同的手势定义
 */
enum class GestureType {
    NONE,
    FORWARD_TILT,      // 向前倾斜
    BACKWARD_TILT,     // 向后倾斜
    SHAKE,             // 摇动
    DOUBLE_TILT,       // 双向倾斜
    LEFT_RIGHT_TILT,   // 左右倾斜 (10秒CD)
    FORWARD_HOLD,      // 前倾保持1秒
    BACKWARD_HOLD,     // 后倾保持1秒
    LEFT_TILT,         // 左倾
    RIGHT_TILT         // 右倾
};

/**
 * @brief IMU 抽象基类
 * 
 * 所有IMU驱动必须实现此接口
 */
class IMUInterface {
public:
    virtual ~IMUInterface() = default;
    
    /**
     * @brief 初始化IMU
     * @return true 初始化成功，false 初始化失败
     */
    virtual bool begin() = 0;
    
    /**
     * @brief 更新传感器数据
     * @param interval 更新间隔（毫秒）
     */
    virtual void update(int interval = 10) = 0;
    
    /**
     * @brief 读取加速度计X轴数据
     * @return 加速度值（原始ADC值）
     */
    virtual int16_t getAccelX() = 0;
    
    /**
     * @brief 读取加速度计Y轴数据
     * @return 加速度值（原始ADC值）
     */
    virtual int16_t getAccelY() = 0;
    
    /**
     * @brief 读取加速度计Z轴数据
     * @return 加速度值（原始ADC值）
     */
    virtual int16_t getAccelZ() = 0;
    
    /**
     * @brief 读取陀螺仪X轴数据
     * @return 角速度值（原始ADC值）
     */
    virtual int16_t getGyroX() = 0;
    
    /**
     * @brief 读取陀螺仪Y轴数据
     * @return 角速度值（原始ADC值）
     */
    virtual int16_t getGyroY() = 0;
    
    /**
     * @brief 读取陀螺仪Z轴数据
     * @return 角速度值（原始ADC值）
     */
    virtual int16_t getGyroZ() = 0;
    
    /**
     * @brief 校准IMU
     * 对于MPU6050：当前未实现（使用固定阈值）
     * 对于QMI8658：可实现零偏校准
     */
    virtual void calibrate() = 0;
    
    /**
     * @brief 获取当前检测到的手势
     * @return 手势类型
     */
    virtual GestureType getGesture() = 0;
    
    /**
     * @brief 重置手势检测状态
     */
    virtual void resetGestureState() = 0;
    
    /**
     * @brief 获取IMU类型名称
     * @return IMU类型字符串
     */
    virtual const char* getTypeName() const = 0;
    
    /**
     * @brief 获取I2C地址
     * @return I2C设备地址
     */
    virtual uint8_t getAddress() const = 0;
};

/**
 * @brief IMU 工厂类
 * 
 * 根据检测到的IMU类型创建对应的实例
 */
class IMUFactory {
public:
    /**
     * @brief 创建IMU实例
     * @param type IMU类型
     * @param address I2C地址
     * @return IMU接口指针，失败返回nullptr
     */
    static IMUInterface* create(HardwareConfig::IMUType type, uint8_t address);
    
private:
    IMUFactory() = default;
};

} // namespace HAL

#endif // IMU_INTERFACE_H
