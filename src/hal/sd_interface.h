/**
 * @file sd_interface.h
 * @brief SD 卡抽象接口
 * 
 * 统一 SDMMC 和 SPI 两种模式的访问接口
 * 支持运行时模式切换和自动回退
 */

#ifndef SD_INTERFACE_H
#define SD_INTERFACE_H

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include "../config/hardware_config.h"

#ifdef PLATFORM_ESP32_S3
    #include <SD_MMC.h>
#endif

namespace HAL {

/**
 * @brief SD 卡抽象接口类
 * 
 * 封装 SDMMC 和 SPI 两种模式的文件操作
 * 对外提供统一的文件系统接口
 */
class SDInterface {
public:
    /**
     * @brief 初始化SD卡
     * @param mode 指定的SD卡模式（可选），如果为FAILED则自动检测
     * @return true 初始化成功，false 初始化失败
     */
    static bool init(HardwareConfig::SDCardMode mode = HardwareConfig::SDCardMode::FAILED);
    
    /**
     * @brief 获取当前SD卡模式
     */
    static HardwareConfig::SDCardMode getMode() { return current_mode_; }
    
    /**
     * @brief 获取模式名称
     */
    static const char* getModeName();
    
    /**
     * @brief 检查SD卡是否已挂载
     */
    static bool isMounted() { return mounted_; }
    
    /**
     * @brief 卸载SD卡
     */
    static void unmount();
    
    /**
     * @brief 获取文件系统接口
     * @return FS& 文件系统引用
     */
    static fs::FS& getFS();
    
    /**
     * @brief 获取SD卡信息
     */
    static void printInfo();
    
    // ========== 文件操作封装 ==========
    
    /**
     * @brief 列出目录内容
     */
    static void listDir(const char* dirname, uint8_t levels);
    
    /**
     * @brief 创建目录
     */
    static void createDir(const char* path);
    
    /**
     * @brief 删除目录
     */
    static void removeDir(const char* path);
    
    /**
     * @brief 读取文件
     */
    static void readFile(const char* path);
    
    /**
     * @brief 读取文件的特定行
     */
    static String readFileLine(const char* path, int num);
    
    /**
     * @brief 写入文件（覆盖模式）
     */
    static void writeFile(const char* path, const char* message);
    
    /**
     * @brief 追加写入文件
     */
    static void appendFile(const char* path, const char* message);
    
    /**
     * @brief 重命名文件
     */
    static void renameFile(const char* path1, const char* path2);
    
    /**
     * @brief 删除文件
     */
    static void deleteFile(const char* path);
    
    /**
     * @brief 读取二进制文件
     */
    static void readBinFromSd(const char* path, uint8_t* buf);
    
    /**
     * @brief 写入二进制文件
     */
    static void writeBinToSd(const char* path, uint8_t* buf);
    
private:
    // 单例模式 - 禁止外部实例化
    SDInterface() = default;
    
    /**
     * @brief 尝试使用 SDMMC 模式初始化
     * @return true 成功，false 失败
     */
    static bool initSDMMC();
    
    /**
     * @brief 尝试使用 SPI 模式初始化
     * @return true 成功，false 失败
     */
    static bool initSPI();
    
    /**
     * @brief 执行SD卡硬件复位
     */
    static void hardwareReset();
    
    // 状态变量
    static HardwareConfig::SDCardMode current_mode_;
    static bool mounted_;
    static SPIClass* spi_instance_;
};

} // namespace HAL

#endif // SD_INTERFACE_H
