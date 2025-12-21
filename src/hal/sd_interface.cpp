/**
 * @file sd_interface.cpp
 * @brief SD 卡抽象接口实现
 * 
 * 统一 SDMMC 和 SPI 两种模式的访问接口
 * 支持运行时模式切换和自动回退
 */

#include "sd_interface.h"
#include "../system/logging/log_manager.h"
#include <SPI.h>

namespace HAL {

// 静态成员初始化
HardwareConfig::SDCardMode SDInterface::current_mode_ = HardwareConfig::SDCardMode::FAILED;
bool SDInterface::mounted_ = false;
SPIClass* SDInterface::spi_instance_ = nullptr;

/**
 * @brief 主初始化函数
 * 
 * 初始化优先级：
 * 1. ESP32-S3: 优先 SDMMC -> 回退 SPI
 * 2. ESP32: 仅 SPI 模式
 */
bool SDInterface::init(HardwareConfig::SDCardMode mode)
{
    LOG_INFO("SD", "Initializing SD card interface...");
    
    // 如果已挂载，先卸载
    if (mounted_) {
        unmount();
    }
    
    // 延迟以让 SD 卡稳定（尤其是在烧录后）
    delay(500);
    
    bool success = false;
    
    // 如果指定了模式，直接尝试该模式
    if (mode != HardwareConfig::SDCardMode::FAILED) {
        if (mode == HardwareConfig::SDCardMode::SDMMC) {
#ifdef PLATFORM_ESP32_S3
            success = initSDMMC();
#else
            LOG_WARN("SD", "SDMMC mode not supported on ESP32, falling back to SPI");
            success = initSPI();
#endif
        } else if (mode == HardwareConfig::SDCardMode::SPI) {
            success = initSPI();
        }
    }
    // 否则自动选择最佳模式
    else {
#ifdef PLATFORM_ESP32_S3
        // ESP32-S3: 仅支持SDMMC模式（硬件无SPI引脚）
        LOG_INFO("SD", "ESP32-S3 detected, using SDMMC mode...");
        success = initSDMMC();
#else
        // ESP32: 仅支持 SPI
        LOG_INFO("SD", "ESP32 detected, using SPI mode");
        success = initSPI();
#endif
    }
    
    if (success) {
        mounted_ = true;
        LOG_INFO("SD", "SD card mounted successfully in " + String(getModeName()) + " mode");
        printInfo();
    } else {
        mounted_ = false;
        current_mode_ = HardwareConfig::SDCardMode::FAILED;
        LOG_ERROR("SD", "SD card initialization failed in all modes");
    }
    
    return success;
}

/**
 * @brief SDMMC 模式初始化
 */
bool SDInterface::initSDMMC()
{
#ifdef PLATFORM_ESP32_S3
    LOG_INFO("SD", "Initializing SD card with SDMMC mode...");
    
    // 硬件复位
    hardwareReset();
    
    // 配置 SDMMC 引脚（1-bit 模式）
    bool success = SD_MMC.setPins(
        HardwareConfig::ESP32S3Pins::SDMMC_CLK,   // CLK
        HardwareConfig::ESP32S3Pins::SDMMC_CMD,   // CMD
        HardwareConfig::ESP32S3Pins::SDMMC_D0     // D0
    );
    
    if (!success) {
        LOG_ERROR("SD", "SDMMC pin configuration failed");
        return false;
    }
    
    delay(100);
    
    // 使用 SDMMC_FREQ_DEFAULT 让驱动自动选择合适频率
    // ESP32-S3: 通常为20MHz（40MHz在S3上有稳定性问题）
    LOG_INFO("SD", "Attempting SDMMC with default frequency (1-bit mode)...");
    
    success = SD_MMC.begin("/sdcard", true, false, SDMMC_FREQ_DEFAULT);
    
    if (success) {
        current_mode_ = HardwareConfig::SDCardMode::SDMMC;
        
        uint8_t cardType = SD_MMC.cardType();
        String typeStr = (cardType == CARD_SD) ? "SDSC" : (cardType == CARD_SDHC) ? "SDHC" : "Unknown";
        
        LOG_INFO("SD", "SDMMC initialized - Card: " + typeStr);
        return true;
    } else {
        LOG_ERROR("SD", "SDMMC mount failed");
        SD_MMC.end();
        return false;
    }
#else
    LOG_ERROR("SD", "SDMMC not supported on this platform");
    return false;
#endif
}

/**
 * @brief SPI 模式初始化
 * 
 * 实现多频率自适应挂载，从高到低尝试
 */
bool SDInterface::initSPI()
{
    LOG_INFO("SD", "Initializing SD card with SPI mode...");
    
    // 硬件复位
    hardwareReset();
    
    // 创建 SPI 实例
    if (spi_instance_ == nullptr) {
#ifdef PLATFORM_ESP32_S3
        spi_instance_ = new SPIClass(FSPI);  // ESP32-S3 使用 FSPI
        spi_instance_->begin(
            HardwareConfig::ESP32S3Pins::SD_SCK,
            HardwareConfig::ESP32S3Pins::SD_MISO,
            HardwareConfig::ESP32S3Pins::SD_MOSI,
            HardwareConfig::ESP32S3Pins::SD_CS
        );
#else
        spi_instance_ = new SPIClass(HSPI);  // ESP32 使用 HSPI
        spi_instance_->begin(
            HardwareConfig::ESP32Pins::SD_SCK,   // SCK=14
            HardwareConfig::ESP32Pins::SD_MISO,  // MISO=26 (避开 GPIO12)
            HardwareConfig::ESP32Pins::SD_MOSI,  // MOSI=13
            HardwareConfig::ESP32Pins::SD_CS     // CS=15
        );
#endif
    }
    
    delay(100);
    
    // 发送至少 74 个时钟脉冲让 SD 卡进入 SPI 模式（SD 规范要求）
    spi_instance_->beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0)); // 低速 400kHz
    for (int i = 0; i < 10; i++) {
        spi_instance_->transfer(0xFF); // 发送 10 字节（80 个时钟）
    }
    spi_instance_->endTransaction();
    delay(100);
    
    // 多频率自适应挂载
    bool mounted = false;
    const uint32_t freq_list[] = {
        25000000,  // 25MHz - ESP32 SPI 最大
        20000000,  // 20MHz
        16000000,  // 16MHz
        12000000,  // 12MHz
        10000000,  // 10MHz
        8000000,   // 8MHz
        5000000,   // 5MHz
        4000000,   // 4MHz
        2000000,   // 2MHz
        1000000    // 1MHz - 最后保底
    };
    const int freq_count = sizeof(freq_list) / sizeof(freq_list[0]);
    
    int cs_pin;
#ifdef PLATFORM_ESP32_S3
    cs_pin = HardwareConfig::ESP32S3Pins::SD_CS;
#else
    cs_pin = HardwareConfig::ESP32Pins::SD_CS;
#endif
    
    for (int attempt = 0; attempt < freq_count; attempt++)
    {
        uint32_t spi_freq = freq_list[attempt];
        LOG_INFO("SD", "Testing SPI at " + String(spi_freq/1000000) + "MHz...");
        
        if (SD.begin(cs_pin, *spi_instance_, spi_freq))
        {
            mounted = true;
            current_mode_ = HardwareConfig::SDCardMode::SPI;
            LOG_INFO("SD", "✓ SPI initialized at " + String(spi_freq/1000000) + "MHz");
            return true;
        }
        
        // 失败后的完整复位流程
        if (attempt < freq_count - 1)
        {
            LOG_WARN("SD", "Failed at " + String(spi_freq/1000000) + "MHz, trying lower speed...");
            SD.end(); // 结束之前的尝试
            
            // 完全复位 SPI 总线和 SD 卡
            spi_instance_->end();
            delay(100);
            
            digitalWrite(cs_pin, LOW);   // CS 拉低
            delay(100);
            digitalWrite(cs_pin, HIGH);  // CS 拉高
            delay(200);
            
#ifdef PLATFORM_ESP32_S3
            spi_instance_->begin(
                HardwareConfig::ESP32S3Pins::SD_SCK,
                HardwareConfig::ESP32S3Pins::SD_MISO,
                HardwareConfig::ESP32S3Pins::SD_MOSI,
                HardwareConfig::ESP32S3Pins::SD_CS
            );
#else
            spi_instance_->begin(
                HardwareConfig::ESP32Pins::SD_SCK,
                HardwareConfig::ESP32Pins::SD_MISO,
                HardwareConfig::ESP32Pins::SD_MOSI,
                HardwareConfig::ESP32Pins::SD_CS
            );
#endif
            delay(100);
            
            // 再次发送时钟脉冲
            spi_instance_->beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));
            for (int i = 0; i < 10; i++) {
                spi_instance_->transfer(0xFF);
            }
            spi_instance_->endTransaction();
            delay(100);
        }
    }
    
    LOG_ERROR("SD", "SPI mount failed at all speeds!");
    return false;
}

/**
 * @brief SD 卡硬件复位
 */
void SDInterface::hardwareReset()
{
    int cs_pin;
    
#ifdef PLATFORM_ESP32_S3
    if (current_mode_ == HardwareConfig::SDCardMode::SDMMC) {
        // SDMMC 模式不需要 CS 复位
        delay(200);
        return;
    }
    cs_pin = HardwareConfig::ESP32S3Pins::SD_CS;
#else
    cs_pin = HardwareConfig::ESP32Pins::SD_CS;
#endif
    
    pinMode(cs_pin, OUTPUT);
    digitalWrite(cs_pin, LOW);   // 先拉低 CS 强制复位 SD 卡
    delay(100);
    digitalWrite(cs_pin, HIGH);  // 拉高 CS 释放 SD 卡
    delay(200);                  // 等待 SD 卡完全复位
}

/**
 * @brief 卸载 SD 卡
 */
void SDInterface::unmount()
{
    if (!mounted_) return;
    
    if (current_mode_ == HardwareConfig::SDCardMode::SDMMC) {
#ifdef PLATFORM_ESP32_S3
        SD_MMC.end();
#endif
    } else if (current_mode_ == HardwareConfig::SDCardMode::SPI) {
        SD.end();
        if (spi_instance_ != nullptr) {
            spi_instance_->end();
            delete spi_instance_;
            spi_instance_ = nullptr;
        }
    }
    
    mounted_ = false;
    current_mode_ = HardwareConfig::SDCardMode::FAILED;
    LOG_INFO("SD", "SD card unmounted");
}

/**
 * @brief 获取模式名称
 */
const char* SDInterface::getModeName()
{
    switch (current_mode_) {
        case HardwareConfig::SDCardMode::SDMMC:
            return "SDMMC";
        case HardwareConfig::SDCardMode::SPI:
            return "SPI";
        case HardwareConfig::SDCardMode::FAILED:
        default:
            return "FAILED";
    }
}

/**
 * @brief 获取文件系统接口
 */
fs::FS& SDInterface::getFS()
{
    if (current_mode_ == HardwareConfig::SDCardMode::SDMMC) {
#ifdef PLATFORM_ESP32_S3
        return SD_MMC;
#else
        return SD;  // 回退到 SD（不应到达这里）
#endif
    } else {
        return SD;  // SPI 模式或失败状态
    }
}

/**
 * @brief 打印 SD 卡信息
 */
void SDInterface::printInfo()
{
    if (!mounted_) {
        LOG_WARN("SD", "Card not mounted");
        return;
    }
    
    fs::FS& fs = getFS();
    uint8_t cardType = 0;
    uint64_t cardSize = 0;
    
    if (current_mode_ == HardwareConfig::SDCardMode::SDMMC) {
#ifdef PLATFORM_ESP32_S3
        cardType = SD_MMC.cardType();
        cardSize = SD_MMC.cardSize() / (1024 * 1024);
#endif
    } else {
        cardType = SD.cardType();
        cardSize = SD.cardSize() / (1024 * 1024);
    }
    
    if (cardType == CARD_NONE) {
        LOG_ERROR("SD", "No SD card attached");
        return;
    }
    
    String typeStr = "UNKNOWN";
    if (cardType == CARD_MMC) typeStr = "MMC";
    else if (cardType == CARD_SD) typeStr = "SDSC";
    else if (cardType == CARD_SDHC) typeStr = "SDHC";
    
    LOG_INFO("SD", "Card Type: " + typeStr);
    LOG_INFO("SD", "Card Size: " + String(cardSize) + "MB");
    LOG_INFO("SD", "Mode: " + String(getModeName()));
    
    Serial.printf("[SD] Card Type: %s\n", typeStr.c_str());
    Serial.printf("[SD] Card Size: %lluMB\n", cardSize);
    Serial.printf("[SD] Mode: %s\n", getModeName());
}

// ========== 文件操作封装 ==========

void SDInterface::listDir(const char* dirname, uint8_t levels)
{
    if (!mounted_) {
        LOG_ERROR("SD", "Card not mounted");
        return;
    }
    
    fs::FS& fs = getFS();
    
    Serial.printf("Listing directory: %s\n", dirname);
    LOG_INFO("SD", "Listing: " + String(dirname));
    
    File root = fs.open(dirname);
    if (!root) {
        LOG_ERROR("SD", "Failed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        LOG_ERROR("SD", "Not a directory");
        return;
    }
    
    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels) {
                listDir(file.path(), levels - 1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void SDInterface::treeDir(const char* dirname, uint8_t levels, const char* prefix)
{
    if (!mounted_) {
        Serial.println("[SD] Card not mounted");
        return;
    }
    
    fs::FS& fs = getFS();
    File root = fs.open(dirname);
    if (!root)
    {
        Serial.printf("%s[Failed to open directory]\n", prefix);
        return;
    }
    if (!root.isDirectory())
    {
        Serial.printf("%s%s [Not a directory]\n", prefix, dirname);
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.printf("%s[DIR]  %s/\n", prefix, file.name());
            if (levels > 0)
            {
                // 构建新的前缀字符串和子目录路径
                String newPrefix = String(prefix) + "|   ";
                String subPath = String(dirname);
                if (!subPath.endsWith("/")) {
                    subPath += "/";
                }
                subPath += file.name();
                treeDir(subPath.c_str(), levels - 1, newPrefix.c_str());
            }
        }
        else
        {
            // 格式化文件大小显示
            size_t fileSize = file.size();
            String sizeStr;
            if (fileSize < 1024) {
                sizeStr = String(fileSize) + "B";
            } else if (fileSize < 1024 * 1024) {
                sizeStr = String(fileSize / 1024) + "KB";
            } else {
                sizeStr = String(fileSize / (1024 * 1024)) + "MB";
            }

            Serial.printf("%s[FILE] %s (%s)\n", prefix, file.name(), sizeStr.c_str());
        }
        file = root.openNextFile();
    }
    root.close();
}

void SDInterface::createDir(const char* path)
{
    if (!mounted_) return;
    
    fs::FS& fs = getFS();
    if (fs.mkdir(path)) {
        LOG_INFO("SD", "Dir created: " + String(path));
    } else {
        LOG_ERROR("SD", "mkdir failed: " + String(path));
    }
}

void SDInterface::removeDir(const char* path)
{
    if (!mounted_) return;
    
    fs::FS& fs = getFS();
    if (fs.rmdir(path)) {
        LOG_INFO("SD", "Dir removed: " + String(path));
    } else {
        LOG_ERROR("SD", "rmdir failed: " + String(path));
    }
}

void SDInterface::readFile(const char* path)
{
    if (!mounted_) return;
    
    fs::FS& fs = getFS();
    File file = fs.open(path);
    if (!file) {
        LOG_ERROR("SD", "Failed to open file: " + String(path));
        return;
    }
    
    Serial.println("Reading: " + String(path));
    while (file.available()) {
        Serial.write(file.read());
    }
    file.close();
}

String SDInterface::readFileLine(const char* path, int num)
{
    if (!mounted_) return "";
    
    fs::FS& fs = getFS();
    File file = fs.open(path);
    if (!file) {
        return "";
    }
    
    String line = "";
    int currentLine = 0;
    while (file.available()) {
        char c = file.read();
        if (c == '\n') {
            currentLine++;
            if (currentLine > num) break;
            if (currentLine == num) {
                file.close();
                return line;
            }
            line = "";
        } else if (c != '\r') {
            line += c;
        }
    }
    
    file.close();
    return (currentLine == num) ? line : "";
}

void SDInterface::writeFile(const char* path, const char* message)
{
    if (!mounted_) return;
    
    fs::FS& fs = getFS();
    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        LOG_ERROR("SD", "Failed to open file for writing: " + String(path));
        return;
    }
    
    if (file.print(message)) {
        LOG_INFO("SD", "File written: " + String(path));
    } else {
        LOG_ERROR("SD", "Write failed: " + String(path));
    }
    file.close();
}

void SDInterface::appendFile(const char* path, const char* message)
{
    if (!mounted_) return;
    
    fs::FS& fs = getFS();
    File file = fs.open(path, FILE_APPEND);
    if (!file) {
        LOG_ERROR("SD", "Failed to open file for appending: " + String(path));
        return;
    }
    
    if (file.print(message)) {
        LOG_DEBUG("SD", "Message appended");
    } else {
        LOG_ERROR("SD", "Append failed: " + String(path));
    }
    file.close();
}

void SDInterface::renameFile(const char* path1, const char* path2)
{
    if (!mounted_) return;
    
    fs::FS& fs = getFS();
    if (fs.rename(path1, path2)) {
        LOG_INFO("SD", "File renamed: " + String(path1) + " -> " + String(path2));
    } else {
        LOG_ERROR("SD", "Rename failed");
    }
}

void SDInterface::deleteFile(const char* path)
{
    if (!mounted_) return;
    
    fs::FS& fs = getFS();
    if (fs.remove(path)) {
        LOG_INFO("SD", "File deleted: " + String(path));
    } else {
        LOG_ERROR("SD", "Delete failed: " + String(path));
    }
}

bool SDInterface::exists(const char* path)
{
    if (!mounted_) return false;
    
    fs::FS& fs = getFS();
    return fs.exists(path);
}

void SDInterface::readBinFromSd(const char* path, uint8_t* buf)
{
    if (!mounted_) return;
    
    fs::FS& fs = getFS();
    File file = fs.open(path);
    if (!file) {
        LOG_ERROR("SD", "Failed to open binary file: " + String(path));
        return;
    }
    
    file.read(buf, file.size());
    file.close();
}

void SDInterface::writeBinToSd(const char* path, uint8_t* buf, size_t size)
{
    if (!mounted_) return;
    
    fs::FS& fs = getFS();
    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        LOG_ERROR("SD", "Failed to open binary file for writing: " + String(path));
        return;
    }
    
    file.write(buf, size);
    file.close();
    LOG_INFO("SD", "Binary file written: " + String(path) + " (" + String(size) + " bytes)");
}

} // namespace HAL
