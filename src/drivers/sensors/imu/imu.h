#ifndef IMU_H
#define IMU_H

#include <I2Cdev.h>
#include <MPU6050.h>

#include "lv_port_indev.h"

#define IMU_I2C_SDA 32
#define IMU_I2C_SCL 33

// 手势类型定义
enum GestureType {
    GESTURE_NONE = 0,
    GESTURE_FORWARD_TILT,     // 向前倾斜
    GESTURE_BACKWARD_TILT,    // 向后倾斜
    GESTURE_SHAKE,            // 摇动
    GESTURE_DOUBLE_TILT,      // 双向倾斜
    GESTURE_LEFT_RIGHT_TILT,  // 左右倾斜 - 触发小鸟（10秒CD）
    GESTURE_FORWARD_HOLD,     // 前倾保持3秒
    GESTURE_BACKWARD_HOLD,    // 后倾保持3秒
    GESTURE_LEFT_TILT,        // 左倾
    GESTURE_RIGHT_TILT        // 右倾
};

extern int32_t encoder_diff;
extern lv_indev_state_t encoder_state;

class IMU
{
private:
    MPU6050 imu;
    int flag;
    int16_t ax, ay, az;
    int16_t gx, gy, gz;

    long last_update_time;
    static bool initialized;

    // 手势检测相关变量
    long last_gesture_time;
    int shake_counter;
    bool was_forward_tilt;
    bool was_backward_tilt;
    int consecutive_tilt_count;

    // 左右倾检测相关变量（简化版）
    unsigned long last_tilt_trigger_time;  // 上次触发时间（用于10秒CD）
    bool was_tilted;                       // 上一帧是否处于倾斜状态

    // 持续手势检测相关变量
    unsigned long forward_hold_start;   // 前倾开始时间
    unsigned long backward_hold_start;  // 后倾开始时间
    unsigned long left_tilt_start;      // 左倾开始时间
    unsigned long right_tilt_start;     // 右倾开始时间
    bool forward_hold_triggered;        // 前倾保持是否已触发
    bool backward_hold_triggered;       // 后倾保持是否已触发

public:
    void init();

    void update(int interval);

    int16_t getAccelX();
    int16_t getAccelY();
    int16_t getAccelZ();

    int16_t getGyroX();
    int16_t getGyroY();
    int16_t getGyroZ();

    // 手势检测方法
    GestureType detectGesture();

private:
    // 手势检测辅助方法
    bool isShaking();
    bool isForwardTilt();
    bool isBackwardTilt();
    bool isLeftOrRightTilt();
    bool isLeftTilt();
    bool isRightTilt();
    void resetGestureState();
};

#endif