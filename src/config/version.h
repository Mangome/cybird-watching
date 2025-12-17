/**
 * @file version.h
 * @brief CybirdWatching 统一版本管理
 *
 * 所有版本号统一在此文件中定义
 */

#ifndef VERSION_H
#define VERSION_H

// 固件版本号 (语义化版本)
#define FIRMWARE_VERSION_MAJOR 1
#define FIRMWARE_VERSION_MINOR 0
#define FIRMWARE_VERSION_PATCH 0

// 版本字符串
#define FIRMWARE_VERSION      "1.0.0"
#define FIRMWARE_VERSION_FULL "v1.0.0"

// 构建信息
#define FIRMWARE_NAME         "CybirdWatching"
#define FIRMWARE_ARCHITECTURE "Dual-Core FreeRTOS"

// 完整版本描述
#define FIRMWARE_BANNER "*** " FIRMWARE_NAME " " FIRMWARE_VERSION_FULL " - " FIRMWARE_ARCHITECTURE " ***"

#endif  // VERSION_H
