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
	ax = data.accel_x_raw;
	ay = data.accel_y_raw;
	az = data.accel_z_raw;
	gx = data.gyro_x_raw;
	gy = data.gyro_y_raw;
	gz = data.gyro_z_raw;

	// Debug output for IMU data
	static unsigned long last_debug_print = 0;
	if (millis() - last_debug_print > 1000) { // 每秒打印一次
		Serial.printf("IMU (%s): ax=%d, ay=%d, az=%d\n", 
			(sensor_type_ == IMUSensorType::QMI8658) ? "QMI8658" : "MPU6050",
			ax, ay, az);
		last_debug_print = millis();
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
	if (!initialized) {
		return GESTURE_NONE;
	}

	unsigned long current_time = millis();

	// 检测持续前倾手势（保持1秒）
	if (isForwardTilt()) {
		if (forward_hold_start == 0) {
			// 开始前倾
			forward_hold_start = current_time;
			forward_hold_triggered = false;
		} else if (!forward_hold_triggered && (current_time - forward_hold_start >= 1000)) {
			// 保持3秒，触发
			forward_hold_triggered = true;
			Serial.println("Gesture detected: FORWARD_HOLD (1s)");
			return GESTURE_FORWARD_HOLD;
		}
	} else {
		// 不再前倾，重置
		forward_hold_start = 0;
		forward_hold_triggered = false;
	}
	
	// 检测持续后倾手势（保持1秒）
	if (isBackwardTilt()) {
		if (backward_hold_start == 0) {
			// 开始后倾
			backward_hold_start = current_time;
			backward_hold_triggered = false;
		} else if (!backward_hold_triggered && (current_time - backward_hold_start >= 1000)) {
			// 保持3秒，触发
			backward_hold_triggered = true;
			Serial.println("Gesture detected: BACKWARD_HOLD (1s)");
			return GESTURE_BACKWARD_HOLD;
		}
	} else {
		// 不再后倾，重置
		backward_hold_start = 0;
		backward_hold_triggered = false;
	}
	
	// 检测左倾手势（0.5秒）
	if (isLeftTilt()) {
		if (left_tilt_start == 0) {
			left_tilt_start = current_time;
		} else if (current_time - left_tilt_start >= 500) {
			// 保持0.5秒，触发
			left_tilt_start = 0;
			Serial.println("Gesture detected: LEFT_TILT");
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
			// 保持0.5秒，触发
			right_tilt_start = 0;
			Serial.println("Gesture detected: RIGHT_TILT");
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
	// 简单的震动检测：加速度变化率
	static int16_t last_ax = 0, last_ay = 0, last_az = 0;

	int16_t delta_ax = abs(ax - last_ax);
	int16_t delta_ay = abs(ay - last_ay);
	int16_t delta_az = abs(az - last_az);

	last_ax = ax;
	last_ay = ay;
	last_az = az;

	// 如果加速度变化很大，认为是摇动
	if (delta_ax > 8000 || delta_ay > 8000 || delta_az > 8000) {
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
	// 前倾：X轴负值较大（ax < -10000）
	return (ax < -10000);
}

// 检测后倾手势
bool IMU::isBackwardTilt()
{
	// 后倾：X轴正值较大（ax > 14000）
	return (ax > 14000);
}

// 检测左倾或右倾手势
bool IMU::isLeftOrRightTilt()
{
	// 根据实际测试数据：
	// 摆正时：ax≈5000,  ay≈-660,   az≈18000
	// 左倾时：ax≈3000,  ay≈12000,  az≈11000
	// 右倾时：ax≈?,     ay≈-12000?, az≈?
	// 
	// 检测条件：Y轴绝对值大于10000（左倾或右倾）
	bool is_tilting = (ay > 10000 || ay < -10000);
	
	if (is_tilting) {
		static unsigned long last_tilt_debug = 0;
		if (millis() - last_tilt_debug > 500) {
			Serial.printf("Left/Right tilt: ax=%d, ay=%d, az=%d\n", ax, ay, az);
			last_tilt_debug = millis();
		}
	}
	
	return is_tilting;
}

// 检测左倾手势
bool IMU::isLeftTilt()
{
	// 左倾：Y轴正值大于10000
	return (ay > 10000);
}

// 检测右倾手势
bool IMU::isRightTilt()
{
	// 右倾：Y轴负值小于-10000
	return (ay < -10000);
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
