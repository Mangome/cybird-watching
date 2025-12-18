## Overview
The QMI8658C Arduino library provides a convenient interface for working with the QMI8658C inertial measurement unit (IMU) on Arduino platforms. The library is designed to simplify the process of reading and interpreting data from the QMI8658C IMU, making it easier for developers to integrate motion sensing capabilities into their projects.
The QMI8658C combines advanced gyroscope and accelerometer capabilities alongside a temperature sensor. Notably, the accelerometer boasts a measurement range of up to ±16g, while the gyroscope offers precision with a range of up to ±2048 degrees per second (dps). 

## IMU
An Inertial Measurement Unit (IMU) is an electronic device that measures and reports information about the movement, orientation, and direction of an object in three-dimensional space. It typically combines three types of sensors: accelerometers, gyroscopes, and magnetometers. By incorporating these sets of sensors, the IMU's degrees of freedom can vary from 3D to 9D. By integrating data from these sensors, an IMU accurately tracks an object's linear acceleration, angular velocity, and orientation relative to Earth's magnetic field, making it essential for applications like navigation, stabilization, and control in autonomous vehicles, drones, and consumer electronics.

### Accelerometer

Accelerometers are sensors that measure linear acceleration, showing how an object's velocity varies over time. They quantify acceleration in meters per second squared (m/s²) or G-forces (g), with one G-force equating to Earth's gravity. These devices utilize technologies like MEMS or piezoelectric materials to achieve precise measurements.

### Gyroscope

A gyroscope, commonly known as a gyro, is a device specialized in measuring or maintaining rotational motion. Gyros are compact and economical sensors used for measuring angular velocity, typically expressed in units of degrees per second (°/s) or revolutions per second (RPS).

## Library functions

Certain public functions are provided by the QMI8658 IMU Library to manage the QMI8658 device, covering tasks like initializing communication, retrieving data, closing communication, and converting operations result to a string for debugging purposes.

### Qmi8658c(uint8_t deviceAddress, uint32_t deviceFrequency)

> Constructor for the Qmi8658c class.

- Parameters:
  - `deviceAddress`: The address of the QMI8658 device.
  - `deviceFrequency`: The communication frequency with the QMI8658 device.

---
### qmi8658_result_t open(qmi8658_cfg_t* qmi8658_cfg)

> Open communication with the QMI8658 device and configure it.

#### Parameters

- `qmi8658_cfg`: Pointer to the configuration settings for the QMI8658 device.

  - `qmi8658_cfg->qmi8658_mode`: Mode of operation for the QMI8658 device.
  - `qmi8658_cfg->acc_scale`: Scale setting for the accelerometer.
  - `qmi8658_cfg->acc_odr`: Output data rate (ODR) setting for the accelerometer.
  - `qmi8658_cfg->gyro_scale`: Scale setting for the gyroscope.
  - `qmi8658_cfg->gyro_odr`: Output data rate (ODR) setting for the gyroscope.

#### Returns

- `qmi8658_result_t`: Result of the operation.

---

### void read(qmi_data_t* data)

> Read data from the QMI8658 device.

#### Parameters

- `data`: Pointer to a structure to store the data read from the QMI8658.

  The `qmi_data_t` structure contains the following fields:
  
  - `data->acc_xyz`: Structure containing linear acceleration (in g) on X, Y, and Z axes.
  - `data->gyro_xyz`: Structure containing angular velocity (in dps) on X, Y, and Z axes.
  - `data->temperature`: Temperature in °C.

#### Returns

- None

---

### qmi8658_result_t close(void)

> Close communication with the QMI8658 device and put it in deep sleep mode.

- Parameters:
  - None

- Returns:
  - `qmi8658_result_t`: Result of the operation.

---

### char* resultToString(qmi8658_result_t result)

> Convert a `qmi8658_result_t` enum value into a corresponding string representation.

- Parameters:
  - `result`: The result enum value to convert.

- Returns:
  - `char*`: String representation of the result.

## Usage

To use the QMI8658 IMU Library in your project, follow these steps:

1. **Include Library**: include the necessary header files in your project.

```c
#include "Qmi8658c.h"

```

2. **Instantiate Object**: create an instance of the `Qmi8658c` class with the desired device address and communication frequency.
```c
#define QMI_ADRESS 0x6b
#define QMI8658C_IIC_FREQUENCY 80*1000 //80kHz

Qmi8658c qmi8658c(QMI_ADRESS, QMI8658C_IIC_FREQUENCY);
```

3. **Open Communication**: use the `open` function to establish communication with the QMI8658 device.

```c
/* QMI8658C configuration */
qmi8658_cfg_t qmi8658_cfg ={
  .qmi8658_mode = qmi8658_mode_acc_only,
  .acc_scale = acc_scale_2g,
  .acc_odr = acc_odr_8000,
  .gyro_scale = gyro_scale_256dps,
  .gyro_odr = gyro_odr_8000,
};

qmi8658_result_t qmi8658_result;

void setup() {
  Serial.begin(9600);
  qmi8658_result = qmi8658c.open(&qmi8658_cfg);
  delay(1000);
  Serial.print("open result : ");
  Serial.println(qmi8658c.resultToString(qmi8658_result));
  Serial.print("deviceID = 0x");
  Serial.println(qmi8658c.deviceID, HEX);
  Serial.print("deviceRevisionID = 0x");
  Serial.println(qmi8658c.deviceRevisionID, HEX);
}

```

4. **Read Data**: use the `read` function to read data from the device. The data will be stored in a `qmi_data_t` structure containing accelerometer, gyroscope, and temperature measurments.
```c
qmi_data_t data;

void loop() {
  qmi8658c.read(&data);
  Serial.print("x: ");
  Serial.print(data.acc_xyz.x);
  Serial.print(" | ");
  Serial.print("y: ");
  Serial.print(data.acc_xyz.y);
  Serial.print(" | ");
  Serial.print("z: ");
  Serial.print(data.acc_xyz.z);
  delay(100);
}
```
5. **Close Communication**: when done, use the `close` function to close communication with the QMI8658 device and put it into low power mode.
```c
  qmi8658_result = qmi8658c.close();
  Serial.print("close result : ");
  Serial.println(qmi8658c.resultToString(qmi8658_result));

