#include "imu.h"
#include "log_manager.h"
#include <esp_task_wdt.h>

bool IMU::initialized = false;
IMUDriver* IMU::driver_ = nullptr;
IMUSensorType IMU::sensor_type_ = IMUSensorType::NONE;

void IMU::init()
{
	LOG_INFO("IMU", "Starting IMU initialization...");
	
	// 喂狗，避免初始化超时
	esp_task_wdt_reset();
	
	// 使用自动检测器检测并创建驱动
	driver_ = IMUDetector::detectAndCreate(IMU_I2C_SDA, IMU_I2C_SCL);
	
	// 再次喂狗
	esp_task_wdt_reset();
	
	if (driver_ != nullptr) {
		sensor_type_ = driver_->getType();
		initialized = true;
		
		const char* sensor_name = (sensor_type_ == IMUSensorType::QMI8658) ? "QMI8658" : "MPU6050";
		LOG_INFO("IMU", String("IMU initialized successfully with ") + sensor_name);
		
		// 初始化手势检测状态
		resetGestureState();
		LOG_INFO("IMU", "Gesture detection initialized");
	} else {
		LOG_ERROR("IMU", "IMU initialization failed - no sensor detected");
		initialized = false;
		sensor_type_ = IMUSensorType::NONE;
	}
}

void IMU::update(int interval)
{
	if (!initialized || driver_ == nullptr) {
		return; // Skip update if IMU is not initialized
	}
	
	// 读取传感器数据
	IMUData data;
	if (!driver_->readData(data)) {
		LOG_ERROR("IMU", "Failed to read sensor data");
		return;
	}
	
	// 更新内部变量（使用原始值）
	// QMI8658 坐标轴映射修正：X/Y 互换，Y 取反
	if (sensor_type_ == IMUSensorType::QMI8658) {
		ax = data.accel_y_raw;   // 原 Y → X（前后倾斜）
		ay = -data.accel_x_raw;  // 原 X → Y（左右倾斜），取反
		az = data.accel_z_raw;
		gx = data.gyro_y_raw;
		gy = -data.gyro_x_raw;
		gz = data.gyro_z_raw;
	} else {
		// MPU6050 保持原样
		ax = data.accel_x_raw;
		ay = data.accel_y_raw;
		az = data.accel_z_raw;
		gx = data.gyro_x_raw;
		gy = data.gyro_y_raw;
		gz = data.gyro_z_raw;
	}

	if (millis() - last_update_time > interval)
	{
		if (ay > 3000 && flag)
		{
			encoder_diff--;
			flag = 0;
			Serial.println("Gesture: Tilt forward - ENCODER--");
		}
		else if (ay < -3000 && flag)
		{
			encoder_diff++;
			flag = 0;
			Serial.println("Gesture: Tilt backward - ENCODER++");
		}
		else
		{
			flag = 1;
		}

		if (ax > 10000)
		{
			encoder_state = LV_INDEV_STATE_PR;
		}
		else
		{
			encoder_state = LV_INDEV_STATE_REL;
		}

		last_update_time = millis();
	}
}

int16_t IMU::getAccelX()
{
	return ax;
}

int16_t IMU::getAccelY()
{
	return ay;
}

int16_t IMU::getAccelZ()
{
	return az;
}

int16_t IMU::getGyroX()
{
	return gx;
}

int16_t IMU::getGyroY()
{
	return gy;
}

int16_t IMU::getGyroZ()
{
	return gz;
}

// 手势检测实现
GestureType IMU::detectGesture()
{
	if (!initialized || driver_ == nullptr) {
		return GESTURE_NONE;
	}

	unsigned long current_time = millis();

	// 检测持续前倾手势（保持1秒）
	if (isForwardTilt()) {
		if (forward_hold_start == 0) {
			forward_hold_start = current_time;
			forward_hold_triggered = false;
		} else if (!forward_hold_triggered && (current_time - forward_hold_start >= 1000)) {
			forward_hold_triggered = true;
			return GESTURE_FORWARD_HOLD;
		}
	} else {
		forward_hold_start = 0;
		forward_hold_triggered = false;
	}
	
	// 检测持续后倾手势（保持1秒）
	if (isBackwardTilt()) {
		if (backward_hold_start == 0) {
			backward_hold_start = current_time;
			backward_hold_triggered = false;
		} else if (!backward_hold_triggered && (current_time - backward_hold_start >= 1000)) {
			backward_hold_triggered = true;
			return GESTURE_BACKWARD_HOLD;
		}
	} else {
		backward_hold_start = 0;
		backward_hold_triggered = false;
	}
	
	// 检测左倾手势（0.5秒）
	if (isLeftTilt()) {
		if (left_tilt_start == 0) {
			left_tilt_start = current_time;
		} else if (current_time - left_tilt_start >= 500) {
			left_tilt_start = 0;
			return GESTURE_LEFT_TILT;
		}
	} else {
		left_tilt_start = 0;
	}
	
	// 检测右倾手势（0.5秒）
	if (isRightTilt()) {
		if (right_tilt_start == 0) {
			right_tilt_start = current_time;
		} else if (current_time - right_tilt_start >= 500) {
			right_tilt_start = 0;
			return GESTURE_RIGHT_TILT;
		}
	} else {
		right_tilt_start = 0;
	}

	return GESTURE_NONE;
}

// 检测摇动手势
bool IMU::isShaking()
{
	if (!initialized || driver_ == nullptr) return false;
	
	// 简单的震动检测：加速度变化率
	static int16_t last_ax = 0, last_ay = 0, last_az = 0;

	int16_t delta_ax = abs(ax - last_ax);
	int16_t delta_ay = abs(ay - last_ay);
	int16_t delta_az = abs(az - last_az);

	last_ax = ax;
	last_ay = ay;
	last_az = az;

	int16_t threshold = driver_->getGestureThresholds().shake;
	
	if (delta_ax > threshold || delta_ay > threshold || delta_az > threshold) {
		shake_counter++;
		if (shake_counter > 3) {
			shake_counter = 0;
			return true;
		}
	} else {
		shake_counter = 0;
	}

	return false;
}

// 检测前倾手势
bool IMU::isForwardTilt()
{
	if (!initialized || driver_ == nullptr) return false;
	return (ax < driver_->getGestureThresholds().forward_tilt);
}

// 检测后倾手势
bool IMU::isBackwardTilt()
{
	if (!initialized || driver_ == nullptr) return false;
	return (ax > driver_->getGestureThresholds().backward_tilt);
}

// 检测左倾或右倾手势
bool IMU::isLeftOrRightTilt()
{
	if (!initialized || driver_ == nullptr) return false;
	
	IMUGestureThresholds thresholds = driver_->getGestureThresholds();
	return (ay > thresholds.left_tilt || ay < thresholds.right_tilt);
}

// 检测左倾手势
bool IMU::isLeftTilt()
{
	if (!initialized || driver_ == nullptr) return false;
	return (ay > driver_->getGestureThresholds().left_tilt);
}

// 检测右倾手势
bool IMU::isRightTilt()
{
	if (!initialized || driver_ == nullptr) return false;
	return (ay < driver_->getGestureThresholds().right_tilt);
}

// 重置手势状态
void IMU::resetGestureState()
{
	last_gesture_time = 0;
	shake_counter = 0;
	was_forward_tilt = false;
	was_backward_tilt = false;
	consecutive_tilt_count = 0;
	
	// 重置左右倾相关状态
	last_tilt_trigger_time = 0;
	was_tilted = false;
	
	// 重置持续手势状态
	forward_hold_start = 0;
	backward_hold_start = 0;
	left_tilt_start = 0;
	right_tilt_start = 0;
	forward_hold_triggered = false;
	backward_hold_triggered = false;
}
