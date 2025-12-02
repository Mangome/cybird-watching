#include <Arduino.h>
#include "serial_commands.h"
#include "log_manager.h"
#include "system/tasks/task_manager.h"

// 前向声明Bird Watching便捷函数
namespace BirdWatching {
    bool triggerBird();
    void showBirdStatistics();
    void listBirds();
    bool isBirdManagerInitialized();
    bool isAnimationPlaying();
    int getStatisticsCount();
}

// 静态成员初始化
SerialCommands* SerialCommands::instance = nullptr;

SerialCommands::SerialCommands() {
    logManager = nullptr;
    commandEnabled = true;
    commandCount = 0;
}

SerialCommands* SerialCommands::getInstance() {
    if (instance == nullptr) {
        instance = new SerialCommands();
    }
    return instance;
}

void SerialCommands::initialize() {
    logManager = LogManager::getInstance();

    // 注册内置命令
    registerCommand("help", "Show available commands");
    registerCommand("log", "Log file operations (clear, size, lines [N], cat) - default shows last 20 lines");
    registerCommand("status", "Show system status");
    registerCommand("clear", "Clear terminal screen");
    registerCommand("tree", "Show SD card directory tree structure [path] [levels]");
    registerCommand("bird", "Bird watching commands (trigger, stats, help)");
    registerCommand("task", "Task monitoring commands (stats, info)");

    LOG_INFO("CMD", "Serial command system initialized");
    Serial.println("Serial command system ready. Type 'help' for available commands.");
}

void SerialCommands::registerCommand(const String& name, const String& description) {
    if (commandCount < MAX_COMMANDS) {
        commands[commandCount].name = name;
        commands[commandCount].description = description;
        commandCount++;
        LOG_DEBUG("CMD", "Registered command: " + name);
    }
}

void SerialCommands::handleInput() {
    if (!commandEnabled) return;

    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();

        if (input.length() == 0) return; // 忽略空行

        LOG_DEBUG("CMD", "Received command: " + input);

        // 解析命令和参数
        String command = input;
        String param = "";

        int spaceIndex = input.indexOf(' ');
        if (spaceIndex > 0) {
            command = input.substring(0, spaceIndex);
            param = input.substring(spaceIndex + 1);
        }

        // 处理命令
        bool commandFound = false;

        if (command.equals("help")) {
            showHelp();
            commandFound = true;
        }
        else if (command.equals("log")) {
            handleLogCommand(param);
            commandFound = true;
        }
        else if (command.equals("status")) {
            handleStatusCommand();
            commandFound = true;
        }
        else if (command.equals("clear")) {
            handleClearCommand();
            commandFound = true;
        }
        else if (command.equals("tree")) {
            handleTreeCommand(param);
            commandFound = true;
        }
        else if (command.equals("bird")) {
            handleBirdCommand(param);
            commandFound = true;
        }
        else if (command.equals("task")) {
            handleTaskCommand(param);
            commandFound = true;
        }

        if (!commandFound) {
            Serial.println("Unknown command: " + command);
            Serial.println("Type 'help' for available commands");
            LOG_WARN("CMD", "Unknown command: " + command);
        }
    }
}

void SerialCommands::handleLogCommand(const String& param) {
    if (param.isEmpty() || param.equals("")) {
        // 检查是否有参数，如果没有参数，显示最后20行
        Serial.println("<<<RESPONSE_START>>>");
        if (logManager) {
            logManager->logToSDOnly(LogManager::LM_LOG_INFO, "CMD", "Showing last 20 lines of log:");
        }
        String logContent = logManager->getLogContent(20);
        Serial.print(logContent); // 使用 print 避免额外换行
        Serial.println("<<<RESPONSE_END>>>");
    }
    else if (param.equals("clear")) {
        Serial.println("<<<RESPONSE_START>>>");
        logManager->clearLogFile();
        Serial.println("Log file cleared");
        Serial.println("<<<RESPONSE_END>>>");
        if (logManager) {
            logManager->logToSDOnly(LogManager::LM_LOG_INFO, "CMD", "Log file cleared by user command");
        }
    }
    else if (param.equals("size")) {
        Serial.println("<<<RESPONSE_START>>>");
        unsigned long size = logManager->getLogFileSize();
        Serial.println("Log file size: " + String(size) + " bytes");
        Serial.println("<<<RESPONSE_END>>>");
        if (logManager) {
            logManager->logToSDOnly(LogManager::LM_LOG_INFO, "CMD", "Log file size queried: " + String(size) + " bytes");
        }
    }
    else if (param.startsWith("lines ")) {
        int lines = param.substring(6).toInt();
        if (lines > 0 && lines <= 500) {
            Serial.println("<<<RESPONSE_START>>>");
            String logContent = logManager->getLogContent(lines);
            Serial.print(logContent); // 使用 print 避免额外换行
            Serial.println("<<<RESPONSE_END>>>");
            if (logManager) {
                logManager->logToSDOnly(LogManager::LM_LOG_INFO, "CMD", "Displayed last " + String(lines) + " lines of log");
            }
        } else {
            Serial.println("<<<RESPONSE_START>>>");
            Serial.println("Invalid line count. Use: log lines 1-500");
            Serial.println("<<<RESPONSE_END>>>");
            LOG_WARN("CMD", "Invalid line count parameter: " + String(lines));
        }
    }
    else if (param.equals("cat") || param.equals("export")) {
        Serial.println("<<<RESPONSE_START>>>");
        Serial.println("=== Full Log File Content ===");

        // 直接顺序读取文件，避免重复读取相同内容
        if (!logManager->isSDCardAvailable()) {
            Serial.println("SD card is not available!");
        } else {
            // 直接打开日志文件顺序读取
            String logFilePath = "/logs/cybird_watching.log";
            if (!SD.exists(logFilePath)) {
                Serial.println("No log file found");
            } else {
                File logFile = SD.open(logFilePath, FILE_READ);
                if (!logFile) {
                    Serial.println("Failed to open log file");
                } else {
                    const int MAX_LINES = 5000; // 最大读取5000行
                    int linesRead = 0;

                    while (logFile.available() && linesRead < MAX_LINES) {
                        String line = logFile.readStringUntil('\n');
                        if (line.length() > 0) {
                            // 限制行长度避免输出过长
                            if (line.length() > 512) {
                                line = line.substring(0, 512) + "...(truncated)";
                            }
                            Serial.println(line);
                            linesRead++;
                        }
                    }

                    logFile.close();

                    if (linesRead >= MAX_LINES) {
                        Serial.println("\n... (Reached maximum read limit of " + String(MAX_LINES) + " lines) ...");
                    }
                }
            }
        }

        Serial.println("=== End of Log File ===");
        Serial.println("<<<RESPONSE_END>>>");
        if (logManager) {
            logManager->logToSDOnly(LogManager::LM_LOG_INFO, "CMD", "Full log file exported");
        }
    }
    else if (param.equals("help")) {
        Serial.println("<<<RESPONSE_START>>>");
        Serial.println("Log subcommands:");
        Serial.println("  (no param)  - Show last 20 lines (default)");
        Serial.println("  clear       - Clear log file");
        Serial.println("  size        - Show log file size");
        Serial.println("  lines N     - Show last N lines (1-500)");
        Serial.println("  cat/export  - Show full log file content");
        Serial.println("  help        - Show this help");
        Serial.println("Examples:");
        Serial.println("  log           - Show last 20 lines");
        Serial.println("  log lines 100 - Show last 100 lines");
        Serial.println("<<<RESPONSE_END>>>");
    }
    else {
        Serial.println("<<<RESPONSE_START>>>");
        Serial.println("Unknown log subcommand: " + param);
        Serial.println("Use 'log help' for available subcommands");
        Serial.println("<<<RESPONSE_END>>>");
        LOG_WARN("CMD", "Unknown log subcommand: " + param);
    }
}

void SerialCommands::handleStatusCommand() {
    // Start response marker
    Serial.println("<<<RESPONSE_START>>>");

    Serial.println("=== CybirdWatching System Status ===");
    Serial.println("Log Manager: " + String(logManager ? "OK" : "FAILED"));
    Serial.println("SD Card: " + String(logManager->isSDCardAvailable() ? "Available" : "Not Available"));
    Serial.println("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("Uptime: " + String(millis() / 1000) + " seconds");

    unsigned long logSize = logManager->getLogFileSize();
    Serial.println("Log file size: " + String(logSize) + " bytes");

    Serial.println("Command system: " + String(commandEnabled ? "Enabled" : "Disabled"));

    // End response marker
    Serial.println("<<<RESPONSE_END>>>");

    // Log to SD card only to avoid interference with command response
    if (logManager) {
        logManager->logToSDOnly(LogManager::LM_LOG_INFO, "CMD", "System status requested");
    }
}


void SerialCommands::handleClearCommand() {
    Serial.println("<<<RESPONSE_START>>>");
    // 发送 ANSI 转义序列清屏
    Serial.println("\033[2J\033[H");
    Serial.println("<<<RESPONSE_END>>>");
    if (logManager) {
        logManager->logToSDOnly(LogManager::LM_LOG_DEBUG, "CMD", "Terminal cleared");
    }
}

void SerialCommands::handleTreeCommand(const String& param) {
    Serial.println("<<<RESPONSE_START>>>");

    // 解析参数
    String path = "/";
    uint8_t levels = 3; // 默认显示3层

    if (!param.isEmpty()) {
        // 分割参数，支持路径和层级两个参数
        int firstSpace = param.indexOf(' ');
        if (firstSpace > 0) {
            // 有两个参数
            path = param.substring(0, firstSpace);
            String levelsStr = param.substring(firstSpace + 1);
            levels = levelsStr.toInt();
            if (levels == 0) levels = 3; // 如果转换失败，使用默认值
            if (levels > 5) levels = 5; // 限制最大层级避免过深显示
        } else {
            // 只有一个参数，判断是路径还是层级
            if (param.charAt(0) == '/' || param.indexOf('/') > 0) {
                // 包含斜杠，认为是路径
                path = param;
            } else {
                // 纯数字，认为是层级
                levels = param.toInt();
                if (levels == 0) levels = 3;
                if (levels > 5) levels = 5;
            }
        }
    }

    Serial.printf("=== SD Card Directory Tree ===\n");
    Serial.printf("Path: %s, Levels: %d\n\n", path.c_str(), levels);

    // 检查SD卡是否可用
    if (!logManager || !logManager->isSDCardAvailable()) {
        Serial.println("SD card is not available!");
        Serial.println("<<<RESPONSE_END>>>");
        return;
    }

    // 调用SD卡类的树状显示方法
    tf.treeDir(path.c_str(), levels, "");

    Serial.println("\n=== End of Tree ===");
    Serial.println("<<<RESPONSE_END>>>");

    if (logManager) {
        logManager->logToSDOnly(LogManager::LM_LOG_INFO, "CMD", "Tree command executed for path: " + path + " with " + String(levels) + " levels");
    }
}

void SerialCommands::showHelp() {
    Serial.println("<<<RESPONSE_START>>>");
    Serial.println("=== Available Commands ===");
    for (int i = 0; i < commandCount; i++) {
        // 格式化命令和描述，确保对齐
        String line = "  " + commands[i].name;
        // 添加填充空格
        while (line.length() < 15) line += " ";
        line += "- " + commands[i].description;
        Serial.println(line);
    }
    Serial.println("===========================");
    Serial.println("Commands format: command [parameter]");
    Serial.println("Example: log lines 100");
    Serial.println("<<<RESPONSE_END>>>");

    if (logManager) {
        logManager->logToSDOnly(LogManager::LM_LOG_INFO, "CMD", "Help command executed");
    }
}

void SerialCommands::setEnabled(bool enabled) {
    commandEnabled = enabled;
    if (logManager) {
        logManager->logToSDOnly(LogManager::LM_LOG_INFO, "CMD", "Command system " + String(enabled ? "enabled" : "disabled"));
    }
}

bool SerialCommands::isEnabled() const {
    return commandEnabled;
}

void SerialCommands::handleBirdCommand(const String& param) {
    Serial.println("<<<RESPONSE_START>>>");

    if (param.isEmpty() || param.equals("help")) {
        Serial.println("Bird watching subcommands:");
        Serial.println("  trigger    - Manually trigger a bird appearance");
        Serial.println("  list       - List all available birds");
        Serial.println("  stats      - Show bird watching statistics");
        Serial.println("  status     - Show bird watching system status");
        Serial.println("  help       - Show this help");
        Serial.println("Examples:");
        Serial.println("  bird trigger  - Trigger a random bird");
        Serial.println("  bird list     - List all birds");
        Serial.println("  bird stats    - Show statistics");
        Serial.println("  bird status   - Show system status");
    }
    else if (param.equals("trigger")) {
        Serial.println("Triggering bird appearance...");
        if (BirdWatching::triggerBird()) {
            Serial.println("Bird triggered successfully!");
        } else {
            Serial.println("Failed to trigger bird. Check if system is initialized.");
        }
    }
    else if (param.equals("list")) {
        Serial.println("=== Available Birds ===");
        BirdWatching::listBirds();
        Serial.println("=== End of List ===");
    }
    else if (param.equals("stats")) {
        Serial.println("=== Bird Watching Statistics ===");
        BirdWatching::showBirdStatistics();
        Serial.println("=== End of Statistics ===");
    }
    else if (param.equals("status")) {
        Serial.println("=== Bird Watching System Status ===");
        if (BirdWatching::isBirdManagerInitialized()) {
            Serial.println("Bird Manager: Initialized");
            Serial.println("Animation System: " + String(BirdWatching::isAnimationPlaying() ? "Playing" : "Idle"));
            Serial.println("Statistics Records: " + String(BirdWatching::getStatisticsCount()));
        } else {
            Serial.println("Bird Manager: NOT INITIALIZED");
        }
        Serial.println("=== End Status ===");
    }
    else {
        Serial.println("Unknown bird subcommand: " + param);
        Serial.println("Use 'bird help' for available subcommands");
    }

    Serial.println("<<<RESPONSE_END>>>");

    if (logManager) {
        logManager->logToSDOnly(LogManager::LM_LOG_INFO, "CMD", "Bird command executed: " + param);
    }
}

void SerialCommands::handleTaskCommand(const String& param) {
    Serial.println("<<<RESPONSE_START>>>");

    if (param.isEmpty() || param.equals("help")) {
        Serial.println("Task monitoring subcommands:");
        Serial.println("  stats      - Show task statistics (stack usage, heap)");
        Serial.println("  info       - Show detailed task information");
        Serial.println("  help       - Show this help");
        Serial.println("Examples:");
        Serial.println("  task stats  - Show task statistics");
        Serial.println("  task info   - Show detailed info");
    }
    else if (param.equals("stats") || param.equals("info")) {
        Serial.println("=== Dual-Core Task Monitor ===");
        
        TaskManager* taskMgr = TaskManager::getInstance();
        if (taskMgr) {
            Serial.println("\n--- Architecture ---");
            Serial.println("Core 0 (Protocol Core):  UI Task");
            Serial.println("  - LVGL GUI (200Hz)");
            Serial.println("  - Display Driver");
            Serial.println("  - Bird Animation");
            Serial.println("");
            Serial.println("Core 1 (Application Core): System Task");
            Serial.println("  - IMU Sensors (5Hz)");
            Serial.println("  - Serial Commands");
            Serial.println("  - Bird Manager Logic");
            Serial.println("  - Statistics");
            
            Serial.println("\n--- Task Statistics ---");
            taskMgr->printTaskStats();
            
            if (param.equals("info")) {
                Serial.println("\n--- FreeRTOS Info ---");
                Serial.printf("Task Count: %d\n", uxTaskGetNumberOfTasks());
                Serial.printf("Min Free Heap Ever: %u bytes\n", ESP.getMinFreeHeap());
                Serial.printf("Heap Fragmentation: %d%%\n", 
                    (int)(100 - (ESP.getMaxAllocHeap() * 100.0 / ESP.getFreeHeap())));
            }
        } else {
            Serial.println("Task Manager not initialized!");
        }
        
        Serial.println("=== End Monitor ===");
    }
    else {
        Serial.println("Unknown task subcommand: " + param);
        Serial.println("Use 'task help' for available subcommands");
    }

    Serial.println("<<<RESPONSE_END>>>");

    if (logManager) {
        logManager->logToSDOnly(LogManager::LM_LOG_INFO, "CMD", "Task command executed: " + param);
    }
}

SerialCommands::~SerialCommands() {
    LOG_DEBUG("CMD", "Serial command system destroyed");
}