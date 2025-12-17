#include "log_manager.h"

#include <Arduino.h>

#include <vector>

#include "sd_card.h"

// 静态成员初始化
LogManager* LogManager::instance = nullptr;

LogManager::LogManager()
{
    sdCardAvailable = false;
    logFilePath = "/logs/cybird_watching.log";
    maxLogFileSize = 1024 * 1024;  // 默认1MB
    currentLogLevel = LM_LOG_INFO;
    logOutputMode = OUTPUT_BOTH;
    lastFlushTime = 0;
}

LogManager* LogManager::getInstance()
{
    if(instance == nullptr) {
        instance = new LogManager();
    }
    return instance;
}

bool LogManager::initialize(LogLevel level, LogOutput output)
{
    currentLogLevel = level;
    logOutputMode = output;

    // 简化初始化：如果是只输出到串口，直接成功
    if(output == OUTPUT_SERIAL) {
        Serial.println("[LOG] LogManager initialized (serial only)");
        return true;
    }

    // 如果需要SD卡，稍后通过setLogOutput再尝试
    if(output == OUTPUT_SD_CARD || output == OUTPUT_BOTH) {
        Serial.println("[LOG] LogManager initialized (SD card support will be checked later)");
        sdCardAvailable = false;  // 先设为false，等SD卡初始化完成后再检查
    }

    return true;
}

bool LogManager::createLogDirectory()
{
    if(!sdCardAvailable) return false;

    // 检查并创建logs目录
    if(!SD.exists("/logs")) {
        if(!SD.mkdir("/logs")) {
            return false;
        }
    }

    return true;
}

void LogManager::checkLogRotation()
{
    if(!sdCardAvailable) return;

    File logFile = SD.open(logFilePath, FILE_READ);
    if(logFile) {
        unsigned long size = logFile.size();
        logFile.close();

        // 如果文件超过最大大小，进行轮转
        if(size > maxLogFileSize) {
            // 删除旧的备份文件
            if(SD.exists(logFilePath + ".old")) {
                SD.remove(logFilePath + ".old");
            }

            // 将当前日志重命名为备份
            SD.rename(logFilePath, logFilePath + ".old");

            if(logOutputMode == OUTPUT_SERIAL || logOutputMode == OUTPUT_BOTH) {
                Serial.println("[LOG] Log rotated, old size: " + String(size) + " bytes");
            }
        }
    }
}

String LogManager::getTimestamp()
{
    unsigned long currentTime = millis();
    unsigned long seconds = currentTime / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;

    unsigned long millis_part = currentTime % 1000;
    unsigned long seconds_part = seconds % 60;
    unsigned long minutes_part = minutes % 60;
    unsigned long hours_part = hours % 24;

    char timestamp[32];
    sprintf(timestamp, "%02lu:%02lu:%02lu.%03lu", hours_part, minutes_part, seconds_part, millis_part);

    return String(timestamp);
}

void LogManager::writeToSDCard(const String& levelStr, const String& tag, const String& message)
{
    if(!sdCardAvailable) return;

    checkLogRotation();

    File logFile = SD.open(logFilePath, FILE_APPEND);
    if(logFile) {
        String logLine = "[" + getTimestamp() + "] [" + levelStr + "] [" + tag + "] " + message + "\n";
        logFile.print(logLine);
        logFile.close();
    }
}

void LogManager::setLogLevel(LogLevel level)
{
    currentLogLevel = level;
}

void LogManager::setLogOutput(LogOutput output)
{
    logOutputMode = output;

    // 如果需要SD卡输出，检查SD卡是否可用（不重新初始化，因为SD卡已经在sd_card.cpp中初始化了）
    if((output == OUTPUT_SD_CARD || output == OUTPUT_BOTH) && !sdCardAvailable) {
        Serial.println("[LOG] Checking SD card availability...");
        // 尝试通过检查SD卡类型来验证SD卡是否可用
        if(SD.cardType() != CARD_NONE) {
            sdCardAvailable = true;
            if(createLogDirectory()) {
                Serial.println("[LOG] SD card is available for logging");
            } else {
                sdCardAvailable = false;
                Serial.println("[LOG] SD card found but cannot create log directory");
            }
        } else {
            Serial.println("[LOG] SD card not available - logging to serial only");
        }
    }
}

LogManager::LogOutput LogManager::getLogOutput()
{
    return logOutputMode;
}

void LogManager::setLogFilePath(const String& path)
{
    logFilePath = path;
}

void LogManager::setMaxLogFileSize(unsigned long size)
{
    maxLogFileSize = size;
}

void LogManager::log(LogLevel level, const String& tag, const String& message)
{
    if(level > currentLogLevel) return;

    String levelStr;
    switch(level) {
        case LM_LOG_FATAL: levelStr = "FATAL"; break;
        case LM_LOG_ERROR: levelStr = "ERROR"; break;
        case LM_LOG_WARN: levelStr = "WARN"; break;
        case LM_LOG_INFO: levelStr = "INFO"; break;
        case LM_LOG_DEBUG: levelStr = "DEBUG"; break;
        case LM_LOG_TRACE: levelStr = "TRACE"; break;
        default: levelStr = "UNKNOWN"; break;
    }

    // 输出到串口
    if(logOutputMode == OUTPUT_SERIAL || logOutputMode == OUTPUT_BOTH) {
        String logLine = "[" + levelStr + "] [" + tag + "] " + message;
        Serial.println(logLine);
    }

    // 输出到SD卡
    if((logOutputMode == OUTPUT_SD_CARD || logOutputMode == OUTPUT_BOTH)) {
        writeToSDCard(levelStr, tag, message);
    }

    // 定期刷新缓冲区
    unsigned long currentTime = millis();
    if(currentTime - lastFlushTime > FLUSH_INTERVAL) {
        flush();
        lastFlushTime = currentTime;
    }
}

void LogManager::debug(const String& tag, const String& message)
{
    log(LM_LOG_DEBUG, tag, message);
}

void LogManager::info(const String& tag, const String& message)
{
    log(LM_LOG_INFO, tag, message);
}

void LogManager::warn(const String& tag, const String& message)
{
    log(LM_LOG_WARN, tag, message);
}

void LogManager::error(const String& tag, const String& message)
{
    log(LM_LOG_ERROR, tag, message);
}

void LogManager::fatal(const String& tag, const String& message)
{
    log(LM_LOG_FATAL, tag, message);
}

void LogManager::logToSDOnly(LogLevel level, const String& tag, const String& message)
{
    if(level > currentLogLevel) return;

    // Only output to SD card, not to serial port
    if(logOutputMode == OUTPUT_SD_CARD || logOutputMode == OUTPUT_BOTH) {
        String levelStr;
        switch(level) {
            case LM_LOG_FATAL: levelStr = "FATAL"; break;
            case LM_LOG_ERROR: levelStr = "ERROR"; break;
            case LM_LOG_WARN: levelStr = "WARN"; break;
            case LM_LOG_INFO: levelStr = "INFO"; break;
            case LM_LOG_DEBUG: levelStr = "DEBUG"; break;
            case LM_LOG_TRACE: levelStr = "TRACE"; break;
            default: levelStr = "UNKNOWN"; break;
        }

        writeToSDCard(levelStr, tag, message);

        // Update flush timer
        unsigned long currentTime = millis();
        if(currentTime - lastFlushTime > FLUSH_INTERVAL) {
            flush();
            lastFlushTime = currentTime;
        }
    }
}

void LogManager::flush()
{
    // 刷新串口缓冲区
    Serial.flush();

    // SD卡文件操作已经自动刷新，不需要额外处理
}

void LogManager::clearLogFile()
{
    if(sdCardAvailable && SD.exists(logFilePath)) {
        SD.remove(logFilePath);
        info("LOG", "Log file cleared");
    }
}

String LogManager::getLogContent(int maxLines)
{
    String content = "";
    if(!sdCardAvailable || !SD.exists(logFilePath)) {
        content = "No log file available\n";
        return content;
    }

    // 限制最大行数以避免内存问题和看门狗超时
    const int safeMaxLines = min(maxLines, 100);

    File logFile = SD.open(logFilePath, FILE_READ);
    if(!logFile) {
        content = "Failed to open log file\n";
        return content;
    }

    unsigned long fileSize = logFile.size();
    if(fileSize == 0) {
        logFile.close();
        content = "Log file is empty\n";
        return content;
    }

    // 策略：从文件末尾开始，向前读取足够的数据以获取最后N行
    // 估算：每行平均100字节，读取 safeMaxLines * 150 字节应该足够
    const size_t readSize = min((size_t)(safeMaxLines * 150), (size_t)fileSize);
    const size_t readPos = (fileSize > readSize) ? (fileSize - readSize) : 0;

    // 移动到读取位置
    logFile.seek(readPos);

    // 如果不是从文件开头读取，跳过第一个不完整的行
    if(readPos > 0) {
        logFile.readStringUntil('\n');  // 丢弃第一个可能不完整的行
    }

    // 使用动态分配的vector
    std::vector<String> lineBuffer;
    lineBuffer.reserve(safeMaxLines);

    int totalLines = 0;
    int linesProcessed = 0;

    // 读取剩余行
    while(logFile.available() && totalLines < safeMaxLines * 2) {  // 读取最多2倍行数以确保足够
        String line = logFile.readStringUntil('\n');
        if(line.length() > 0) {
            // 限制行长度避免内存问题
            if(line.length() > 256) {
                line = line.substring(0, 256) + "...";
            }

            // 使用环形缓冲区逻辑
            if(lineBuffer.size() < safeMaxLines) {
                lineBuffer.push_back(line);
            } else {
                // 移除最旧的行，添加新行
                lineBuffer.erase(lineBuffer.begin());
                lineBuffer.push_back(line);
            }
            totalLines++;
        }

        // 每处理5行，让出CPU并喂狗
        linesProcessed++;
        if(linesProcessed % 5 == 0) {
            yield();
        }
    }
    logFile.close();

    // 构建结果
    int linesToShow = lineBuffer.size();
    content = "=== Last " + String(linesToShow) + " lines ===\n";

    for(int i = 0; i < linesToShow; i++) {
        if(lineBuffer[i].length() > 0) {
            content += lineBuffer[i] + "\n";
        }
    }

    if(content.length() == 0) {
        content = "Log file is empty or unreadable\n";
    }

    return content;
}

unsigned long LogManager::getLogFileSize()
{
    if(!sdCardAvailable || !SD.exists(logFilePath)) {
        return 0;
    }

    File logFile = SD.open(logFilePath, FILE_READ);
    if(logFile) {
        unsigned long size = logFile.size();
        logFile.close();
        return size;
    }

    return 0;
}

bool LogManager::isSDCardAvailable() const
{
    return sdCardAvailable;
}

void LogManager::shutdown()
{
    flush();
}

LogManager::~LogManager()
{
    shutdown();
}