#include "imu.h"
#include "log_manager.h"

bool IMU::initialized = false;

void IMU::init()
{
	LOG_INFO("IMU", "Starting I2C...");
	Wire.begin(IMU_I2C_SDA, IMU_I2C_SCL);
	Wire.setClock(100000); // Reduce clock speed for more reliable communication

	LOG_INFO("IMU", "Scanning I2C bus...");
	byte error, address;
	int nDevices = 0;

	for(address = 1; address < 127; address++) {
		Wire.beginTransmission(address);
		error = Wire.endTransmission();

		if (error == 0) {
			LOG_INFO("IMU", "I2C device found at address 0x" + String(address, HEX));
			nDevices++;
		}
	}

	if (nDevices == 0) {
		LOG_ERROR("IMU", "No I2C devices found - MPU may not be connected");
		initialized = false;
		return;
	} else {
		LOG_INFO("IMU", "Found " + String(nDevices) + " I2C device(s)");
	}

	// MPU6050 was found at 0x68, try direct I2C communication
	LOG_INFO("IMU", "Testing direct I2C communication with MPU6050...");

	// Try to read WHO_AM_I register directly via I2C
	Wire.beginTransmission(0x68);
	Wire.write(0x75); // WHO_AM_I register
	Wire.endTransmission(false);

	Wire.requestFrom(0x68, 1);
	if (Wire.available()) {
		uint8_t whoami = Wire.read();
		Serial.printf("  MPU WHO_AM_I register: 0x%02X (expected: 0x68)\n", whoami);

		if (whoami == 0x68) {
			Serial.println("  MPU6050 communication OK, initializing manually...");

			// Wake up MPU6050 and configure it
			Serial.println("  Waking up MPU6050...");
			Wire.beginTransmission(0x68);
			Wire.write(0x6B); // PWR_MGMT_1 register
			Wire.write(0x00); // Wake up
			int result = Wire.endTransmission();
			Serial.printf("  Wake up result: %d\n", result);

			delay(100); // Wait for MPU to wake up

			// Configure accelerometer
			Serial.println("  Configuring accelerometer...");
			Wire.beginTransmission(0x68);
			Wire.write(0x1C); // ACCEL_CONFIG register
			Wire.write(0x00); // ±2g range
			result = Wire.endTransmission();
			Serial.printf("  Accelerometer config result: %d\n", result);

			Serial.println("  MPU6050 manual initialization complete");
			initialized = true;

			// 初始化手势检测状态
			resetGestureState();
			Serial.println("  Gesture detection initialized");
		} else {
			Serial.printf("  Unexpected WHO_AM_I value: 0x%02X\n", whoami);
			initialized = false;
		}
	} else {
		Serial.println("  Failed to read WHO_AM_I register");
		initialized = false;
	}
}

void IMU::update(int interval)
{
	if (!initialized) {
		return; // Skip update if MPU is not initialized
	}
	Wire.beginTransmission(0x68);
	Wire.write(0x3B); // Start from ACCEL_XOUT_H
	int result = Wire.endTransmission(false);

	if (result != 0) {
		Serial.printf("  I2C transmission error: %d\n", result);
		return;
	}
	Wire.requestFrom(0x68, 6); // Read 6 bytes for accelerometer

	int bytes_received = Wire.available();

	if (bytes_received >= 6) {
		uint8_t data[6];
		for (int i = 0; i < 6; i++) {
			data[i] = Wire.read();
		}

		int16_t ax_raw = (data[0] << 8) | data[1];
		int16_t ay_raw = (data[2] << 8) | data[3];
		int16_t az_raw = (data[4] << 8) | data[5];

		ax = ax_raw;
		ay = ay_raw;
		az = az_raw;

		// Simple gyroscope read (optional for basic functionality)
		gx = 0;
		gy = 0;
		gz = 0;

		// Debug output for MPU data - 用于调试左倾方向
		static unsigned long last_debug_print = 0;
		if (millis() - last_debug_print > 1000) { // 每秒打印一次
			Serial.printf("MPU: ax=%d, ay=%d, az=%d\n", ax, ay, az);
			last_debug_print = millis();
		}
	} else {
		Serial.printf("  Failed to read MPU data, only got %d bytes\n", bytes_received);
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

	// 优先检测左右倾斜手势（单次触发，10秒CD）
	bool is_tilting_now = isLeftOrRightTilt();
	
	// 检测倾斜状态的变化：从不倾斜到倾斜（上升沿触发）
	if (is_tilting_now && !was_tilted) {
		// 刚开始倾斜，检查CD时间
		unsigned long time_since_last_trigger = current_time - last_tilt_trigger_time;
		
		if (time_since_last_trigger >= 10000) {  // 10秒CD
			// CD已过，可以触发
			last_tilt_trigger_time = current_time;
			was_tilted = true;
			Serial.println("Gesture detected: LEFT_RIGHT_TILT");
			return GESTURE_LEFT_RIGHT_TILT;
		} else {
			// CD未过，提示剩余时间
			unsigned long remaining = 10000 - time_since_last_trigger;
			Serial.printf("Tilt detected but CD active, %lu ms remaining\n", remaining);
			was_tilted = true;  // 更新状态避免重复提示
			return GESTURE_NONE;
		}
	} else if (!is_tilting_now && was_tilted) {
		// 从倾斜恢复到正常，重置状态
		was_tilted = false;
		Serial.println("Tilt ended");
	}

	// 手势冷却时间，避免重复触发
	if (current_time - last_gesture_time < 1000) {
		return GESTURE_NONE;
	}

	GestureType detected = GESTURE_NONE;
	// 检测前倾手势
	if (isForwardTilt()) {
		detected = GESTURE_FORWARD_TILT;
		Serial.println("Gesture detected: FORWARD_TILT");

		// 记录倾斜序列，用于检测双向倾斜
		if (was_backward_tilt && (current_time - last_gesture_time < 2000)) {
			consecutive_tilt_count++;
			if (consecutive_tilt_count >= 3) {
				detected = GESTURE_DOUBLE_TILT;
				Serial.println("Gesture detected: DOUBLE_TILT (sequence completed)");
			}
		} else {
			was_forward_tilt = true;
			was_backward_tilt = false;
			consecutive_tilt_count = 1;
		}
	}
	// 检测后倾手势
	else if (isBackwardTilt()) {
		detected = GESTURE_BACKWARD_TILT;
		Serial.println("Gesture detected: BACKWARD_TILT");

		// 记录倾斜序列
		if (was_forward_tilt && (current_time - last_gesture_time < 2000)) {
			consecutive_tilt_count++;
			if (consecutive_tilt_count >= 3) {
				detected = GESTURE_DOUBLE_TILT;
				Serial.println("Gesture detected: DOUBLE_TILT (sequence completed)");
			}
		} else {
			was_backward_tilt = true;
			was_forward_tilt = false;
			consecutive_tilt_count = 1;
		}
	}

	// 更新手势时间
	if (detected != GESTURE_NONE) {
		last_gesture_time = current_time;
	}

	return detected;
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
	// 前倾：Y轴负值较大
	return (ay < -5000);
}

// 检测后倾手势
bool IMU::isBackwardTilt()
{
	// 后倾：Y轴正值较大
	return (ay > 5000);
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
}
