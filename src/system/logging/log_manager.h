#pragma once

#include <Arduino.h>
#include <FS.h>
#include <SD.h>

class LogManager
{
public:
    enum LogLevel {
        LM_LOG_SILENT = 0,
        LM_LOG_FATAL = 1,
        LM_LOG_ERROR = 2,
        LM_LOG_WARN = 3,
        LM_LOG_INFO = 4,
        LM_LOG_DEBUG = 5,
        LM_LOG_TRACE = 6
    };

    enum LogOutput { OUTPUT_SERIAL = 1, OUTPUT_SD_CARD = 2, OUTPUT_BOTH = 3 };

private:
    static LogManager* instance;
    bool sdCardAvailable;
    String logFilePath;
    unsigned long maxLogFileSize;
    int currentLogLevel;
    LogOutput logOutputMode;
    unsigned long lastFlushTime;
    const unsigned long FLUSH_INTERVAL = 5000;  // 5秒刷新一次

    // 私有构造函数，单例模式
    LogManager();

    // 创建日志文件目录
    bool createLogDirectory();

    // 检查并执行日志轮转
    void checkLogRotation();

    // 获取格式化的时间戳字符串
    String getTimestamp();

    // 写入日志到SD卡
    void writeToSDCard(const String& levelStr, const String& tag, const String& message);

public:
    // 获取单例实例
    static LogManager* getInstance();

    // 初始化日志系统
    bool initialize(LogLevel level = LM_LOG_INFO, LogOutput output = OUTPUT_BOTH);

    // 设置日志级别
    void setLogLevel(LogLevel level);

    // 设置日志输出模式
    void setLogOutput(LogOutput output);

    // 获取当前日志输出模式
    LogOutput getLogOutput();

    // 设置日志文件路径
    void setLogFilePath(const String& path);

    // 设置最大日志文件大小（字节）
    void setMaxLogFileSize(unsigned long size);

    // 日志记录方法
    void log(LogLevel level, const String& tag, const String& message);
    void debug(const String& tag, const String& message);
    void info(const String& tag, const String& message);
    void warn(const String& tag, const String& message);
    void error(const String& tag, const String& message);
    void fatal(const String& tag, const String& message);

    // 仅记录到SD卡，不输出到串口（用于避免干扰命令响应）
    void logToSDOnly(LogLevel level, const String& tag, const String& message);

    // 刷新缓冲区
    void flush();

    // 清空日志文件
    void clearLogFile();

    // 获取日志文件内容（用于串口命令）
    String getLogContent(int maxLines = 100);

    // 获取日志文件大小
    unsigned long getLogFileSize();

    // 检查SD卡是否可用
    bool isSDCardAvailable() const;

    // 关闭日志系统
    void shutdown();

    // 析构函数
    ~LogManager();
};

// 全局日志宏定义，方便使用
#define LOG_DEBUG(tag, msg) LogManager::getInstance()->debug(tag, msg)
#define LOG_INFO(tag, msg)  LogManager::getInstance()->info(tag, msg)
#define LOG_WARN(tag, msg)  LogManager::getInstance()->warn(tag, msg)
#define LOG_ERROR(tag, msg) LogManager::getInstance()->error(tag, msg)
#define LOG_FATAL(tag, msg) LogManager::getInstance()->fatal(tag, msg)