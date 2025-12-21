#pragma once

#include <Arduino.h>
#include "log_manager.h"
#include "hal/sd_interface.h"

class SerialCommands {
private:
    static const int MAX_COMMANDS = 20;

    struct Command {
        String name;
        String description;
    };

    static SerialCommands* instance;
    Command commands[MAX_COMMANDS];
    int commandCount;
    LogManager* logManager;
    bool commandEnabled;

    // 私有构造函数，单例模式
    SerialCommands();

public:
    // 获取单例实例
    static SerialCommands* getInstance();

    // 初始化串口命令系统
    void initialize();

    // 注册新命令
    void registerCommand(const String& name, const String& description);

    // 处理串口输入
    void handleInput();

    // 显示帮助信息
    void showHelp();

    // 设置命令系统开关
    void setEnabled(bool enabled);

    // 检查是否启用
    bool isEnabled() const;

    // 析构函数
    ~SerialCommands();

private:
    // 命令处理函数
    void handleLogCommand(const String& param);
    void handleStatusCommand();
    void handleClearCommand();
    void handleTreeCommand(const String& param);
    void handleBirdCommand(const String& param);
    void handleTaskCommand(const String& param);
    void handleFileCommand(const String& param);
    
    // 文件传输辅助函数
    void handleFileUpload(const String& param);
    void handleFileDownload(const String& param);
    void handleFileDelete(const String& param);
    void handleFileInfo(const String& param);
    String base64Encode(const uint8_t* data, size_t length);
    size_t base64Decode(const String& input, uint8_t* output, size_t maxLength);
};