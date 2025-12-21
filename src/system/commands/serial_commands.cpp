#include <Arduino.h>
#include "serial_commands.h"
#include "log_manager.h"
#include "system/tasks/task_manager.h"
#include "config/version.h"

// 前向声明Bird Watching便捷函数
namespace BirdWatching {
    bool triggerBird(uint16_t bird_id = 0);
    void showBirdStatistics();
    bool resetBirdStatistics();
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
    registerCommand("file", "File transfer commands (upload, download, delete, info)");

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
        else if (command.equals("file")) {
            handleFileCommand(param);
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
            if (!HAL::SDInterface::exists(logFilePath.c_str())) {
                Serial.println("No log file found");
            } else {
                fs::FS& fs = HAL::SDInterface::getFS();
                File logFile = fs.open(logFilePath, FILE_READ);
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
    Serial.println("Firmware: " + String(FIRMWARE_VERSION_FULL));
    Serial.println("Architecture: " + String(FIRMWARE_ARCHITECTURE));
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


    // 调用 HAL::SDInterface 的树状显示方法
    HAL::SDInterface::treeDir(path.c_str(), levels, "");

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
        Serial.println("  trigger [id] - Manually trigger a bird appearance (random if no id)");
        Serial.println("  list         - List all available birds");
        Serial.println("  stats        - Show bird watching statistics");
        Serial.println("  status       - Show bird watching system status");
        Serial.println("  reset        - Reset bird watching statistics and save to file");
        Serial.println("  help         - Show this help");
        Serial.println("Examples:");
        Serial.println("  bird trigger      - Trigger a random bird");
        Serial.println("  bird trigger 1001 - Trigger bird with ID 1001");
        Serial.println("  bird list         - List all birds");
        Serial.println("  bird stats        - Show statistics");
        Serial.println("  bird status       - Show system status");
        Serial.println("  bird reset        - Reset all statistics");
    }
    else if (param.equals("trigger") || param.startsWith("trigger ")) {
        uint16_t bird_id = 0;
        
        // 检查是否有指定小鸟ID
        if (param.startsWith("trigger ")) {
            String id_str = param.substring(8); // 获取 "trigger " 后面的内容
            id_str.trim();
            bird_id = id_str.toInt();
            
            if (bird_id > 0) {
                Serial.println("Triggering bird ID " + String(bird_id) + "...");
            } else {
                Serial.println("Invalid bird ID: " + id_str);
                Serial.println("Use 'bird list' to see available bird IDs");
                return;
            }
        } else {
            Serial.println("Triggering random bird appearance...");
        }
        
        if (BirdWatching::triggerBird(bird_id)) {
            if (bird_id > 0) {
                Serial.println("Bird ID " + String(bird_id) + " triggered successfully!");
            } else {
                Serial.println("Random bird triggered successfully!");
            }
        } else {
            Serial.println("Failed to trigger bird. Check if system is initialized or bird ID exists.");
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
    else if (param.equals("reset")) {
        Serial.println("Resetting bird watching statistics...");
        if (BirdWatching::resetBirdStatistics()) {
            Serial.println("Statistics reset successfully and saved to file!");
        } else {
            Serial.println("Failed to reset statistics. Check if system is initialized.");
        }
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

// ============================================
// 文件传输命令实现
// ============================================

void SerialCommands::handleFileCommand(const String& param) {
    Serial.println("<<<RESPONSE_START>>>");

    if (param.isEmpty() || param.equals("help")) {
        Serial.println("File transfer subcommands:");
        Serial.println("  upload <path>   - Upload file to SD card (receives base64 data)");
        Serial.println("  download <path> - Download file from SD card (sends base64 data)");
        Serial.println("  delete <path>   - Delete file from SD card");
        Serial.println("  info <path>     - Show file information");
        Serial.println("  help            - Show this help");
        Serial.println("\nUpload protocol:");
        Serial.println("  1. Send: file upload /path/to/file.bin");
        Serial.println("  2. Wait for READY prompt");
        Serial.println("  3. Send: FILE_SIZE:<bytes>");
        Serial.println("  4. Send base64 encoded data in chunks (max 512 bytes/line)");
        Serial.println("  5. Send: FILE_END");
        Serial.println("\nExamples:");
        Serial.println("  file download /configs/bird_config.csv");
        Serial.println("  file info /birds/1001/1.bin");
        Serial.println("  file delete /temp/old_file.txt");
    }
    else if (param.startsWith("upload ")) {
        String path = param.substring(7);
        path.trim();
        handleFileUpload(path);
    }
    else if (param.startsWith("download ")) {
        String path = param.substring(9);
        path.trim();
        handleFileDownload(path);
    }
    else if (param.startsWith("delete ")) {
        String path = param.substring(7);
        path.trim();
        handleFileDelete(path);
    }
    else if (param.startsWith("info ")) {
        String path = param.substring(5);
        path.trim();
        handleFileInfo(path);
    }
    else {
        Serial.println("Unknown file subcommand");
        Serial.println("Use 'file help' for available subcommands");
    }

    Serial.println("<<<RESPONSE_END>>>");

    if (logManager) {
        logManager->logToSDOnly(LogManager::LM_LOG_INFO, "CMD", "File command executed: " + param);
    }
}

void SerialCommands::handleFileUpload(const String& path) {
    if (!logManager || !logManager->isSDCardAvailable()) {
        Serial.println("ERROR: SD card not available");
        return;
    }

    // 确保目录存在
    String dirPath = path;
    int lastSlash = dirPath.lastIndexOf('/');
    if (lastSlash > 0) {
        dirPath = dirPath.substring(0, lastSlash);
        fs::FS& fs = HAL::SDInterface::getFS();
        if (!fs.exists(dirPath)) {
            // 创建目录结构
            String currentPath = "";
            int start = 1; // 跳过开头的 '/'
            while (start < dirPath.length()) {
                int nextSlash = dirPath.indexOf('/', start);
                if (nextSlash == -1) nextSlash = dirPath.length();
                
                currentPath += "/" + dirPath.substring(start, nextSlash);
                if (!fs.exists(currentPath)) {
                    if (!fs.mkdir(currentPath)) {
                        Serial.println("ERROR: Failed to create directory: " + currentPath);
                        return;
                    }
                }
                start = nextSlash + 1;
            }
        }
    }

    Serial.println("READY");
    Serial.println("Waiting for file data...");
    Serial.println("Send FILE_SIZE:<bytes> first, then base64 data, end with FILE_END");

    // 等待文件大小
    unsigned long timeout = millis() + 30000; // 30秒超时
    size_t expectedSize = 0;
    bool sizeReceived = false;

    while (millis() < timeout && !sizeReceived) {
        if (Serial.available()) {
            String line = Serial.readStringUntil('\n');
            line.trim();
            
            if (line.startsWith("FILE_SIZE:")) {
                expectedSize = line.substring(10).toInt();
                sizeReceived = true;
                Serial.printf("Expecting %u bytes\n", expectedSize);
            }
        }
        delay(10);
    }

    if (!sizeReceived) {
        Serial.println("ERROR: Timeout waiting for FILE_SIZE");
        return;
    }

    // 打开文件准备写入
    fs::FS& fs = HAL::SDInterface::getFS();
    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("ERROR: Failed to create file: " + path);
        return;
    }

    // 接收并解码数据
    String base64Buffer = "";
    size_t totalWritten = 0;
    bool transferComplete = false;
    timeout = millis() + 120000; // 2分钟超时

    while (millis() < timeout && !transferComplete) {
        if (Serial.available()) {
            String line = Serial.readStringUntil('\n');
            line.trim();

            if (line.equals("FILE_END")) {
                transferComplete = true;
                break;
            }

            // 累积base64数据
            base64Buffer += line;

            // 每1KB解码一次（base64编码后约1365字符）
            if (base64Buffer.length() >= 1360) {
                uint8_t decoded[1024];
                size_t decodedLen = base64Decode(base64Buffer.substring(0, 1364), decoded, sizeof(decoded));
                
                if (decodedLen > 0) {
                    size_t written = file.write(decoded, decodedLen);
                    totalWritten += written;
                    Serial.printf("Progress: %u / %u bytes (%.1f%%)\n", 
                        totalWritten, expectedSize, 
                        (totalWritten * 100.0) / expectedSize);
                }
                
                base64Buffer = base64Buffer.substring(1364);
                timeout = millis() + 120000; // 重置超时
            }
        }
        delay(1);
    }

    // 处理剩余数据
    if (base64Buffer.length() > 0) {
        uint8_t decoded[1024];
        size_t decodedLen = base64Decode(base64Buffer, decoded, sizeof(decoded));
        if (decodedLen > 0) {
            totalWritten += file.write(decoded, decodedLen);
        }
    }

    file.close();

    if (transferComplete) {
        Serial.printf("SUCCESS: File uploaded successfully!\n");
        Serial.printf("Path: %s\n", path.c_str());
        Serial.printf("Size: %u bytes\n", totalWritten);
    } else {
        Serial.println("ERROR: Transfer timeout or incomplete");
        fs.remove(path); // 删除不完整的文件
    }
}

void SerialCommands::handleFileDownload(const String& path) {
    if (!logManager || !logManager->isSDCardAvailable()) {
        Serial.println("ERROR: SD card not available");
        return;
    }

    fs::FS& fs = HAL::SDInterface::getFS();
    if (!fs.exists(path)) {
        Serial.println("ERROR: File not found: " + path);
        return;
    }

    File file = fs.open(path, FILE_READ);
    if (!file) {
        Serial.println("ERROR: Failed to open file: " + path);
        return;
    }

    size_t fileSize = file.size();
    Serial.printf("FILE_START:%s:%u\n", path.c_str(), fileSize);

    // 分块读取并base64编码发送
    const size_t CHUNK_SIZE = 768; // 768字节编码后刚好1024字符
    uint8_t buffer[CHUNK_SIZE];
    size_t totalSent = 0;

    while (file.available()) {
        size_t bytesRead = file.read(buffer, CHUNK_SIZE);
        if (bytesRead > 0) {
            String encoded = base64Encode(buffer, bytesRead);
            Serial.println(encoded);
            totalSent += bytesRead;
            
            // 显示进度
            if (totalSent % (CHUNK_SIZE * 10) == 0 || totalSent == fileSize) {
                Serial.printf("PROGRESS:%u/%u\n", totalSent, fileSize);
            }
        }
        yield(); // 喂狗
    }

    file.close();
    Serial.println("FILE_END");
    Serial.printf("SUCCESS: %u bytes sent\n", totalSent);
}

void SerialCommands::handleFileDelete(const String& path) {
    if (!logManager || !logManager->isSDCardAvailable()) {
        Serial.println("ERROR: SD card not available");
        return;
    }

    fs::FS& fs = HAL::SDInterface::getFS();
    if (!fs.exists(path)) {
        Serial.println("ERROR: File not found: " + path);
        return;
    }

    if (fs.remove(path)) {
        Serial.println("SUCCESS: File deleted: " + path);
    } else {
        Serial.println("ERROR: Failed to delete file: " + path);
    }
}

void SerialCommands::handleFileInfo(const String& path) {
    if (!logManager || !logManager->isSDCardAvailable()) {
        Serial.println("ERROR: SD card not available");
        return;
    }

    fs::FS& fs = HAL::SDInterface::getFS();
    if (!fs.exists(path)) {
        Serial.println("ERROR: File not found: " + path);
        return;
    }

    File file = fs.open(path, FILE_READ);
    if (!file) {
        Serial.println("ERROR: Failed to open file: " + path);
        return;
    }

    Serial.println("=== File Information ===");
    Serial.printf("Path: %s\n", path.c_str());
    Serial.printf("Size: %u bytes (%.2f KB)\n", file.size(), file.size() / 1024.0);
    Serial.printf("Type: %s\n", file.isDirectory() ? "Directory" : "File");
    
    file.close();
    Serial.println("========================");
}

// Base64编码实现
String SerialCommands::base64Encode(const uint8_t* data, size_t length) {
    const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    String result;
    result.reserve((length + 2) / 3 * 4);

    for (size_t i = 0; i < length; i += 3) {
        uint32_t b = (data[i] << 16) | 
                     ((i + 1 < length ? data[i + 1] : 0) << 8) |
                     (i + 2 < length ? data[i + 2] : 0);

        result += base64_chars[(b >> 18) & 0x3F];
        result += base64_chars[(b >> 12) & 0x3F];
        result += (i + 1 < length) ? base64_chars[(b >> 6) & 0x3F] : '=';
        result += (i + 2 < length) ? base64_chars[b & 0x3F] : '=';
    }

    return result;
}

// Base64解码实现
size_t SerialCommands::base64Decode(const String& input, uint8_t* output, size_t maxLength) {
    const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    size_t outLen = 0;
    uint32_t buffer = 0;
    int bits = 0;

    for (size_t i = 0; i < input.length() && outLen < maxLength; i++) {
        char c = input[i];
        if (c == '=') break;

        const char* p = strchr(base64_chars, c);
        if (!p) continue;

        buffer = (buffer << 6) | (p - base64_chars);
        bits += 6;

        if (bits >= 8) {
            bits -= 8;
            output[outLen++] = (buffer >> bits) & 0xFF;
        }
    }

    return outLen;
}