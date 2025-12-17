#include "sd_card.h"

#include "log_manager.h"

void SdCard::init()
{
    LOG_INFO("SD", "Initializing SD card with HSPI...");

    // 延迟以让SD卡稳定（尤其是在烧录后）
    delay(500);

    // Create HSPI instance with custom MISO pin 26 to avoid GPIO12 boot issue
    SPIClass* sd_spi = new SPIClass(HSPI);  // another SPI

    // 【关键】完全复位SD卡和SPI总线
    pinMode(15, OUTPUT);
    digitalWrite(15, LOW);  // 先拉低CS强制复位SD卡
    delay(100);
    digitalWrite(15, HIGH);  // 拉高CS释放SD卡
    delay(200);              // 等待SD卡完全复位

    // 初始化SPI总线
    sd_spi->begin(14, 26, 13, 15);  // SCK=14, MISO=26, MOSI=13, SS=15
    delay(100);

    // 【新增】发送至少74个时钟脉冲让SD卡进入SPI模式（SD规范要求）
    sd_spi->beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));  // 低速400kHz
    for(int i = 0; i < 10; i++) {
        sd_spi->transfer(0xFF);  // 发送10字节（80个时钟）
    }
    sd_spi->endTransaction();
    delay(100);

    // 扩展重试机制：测试多个频率找到最佳速度
    bool mounted = false;
    const uint32_t freq_list[] = {
        25000000,  // 25MHz - ESP32 SPI最大
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

    for(int attempt = 0; attempt < freq_count; attempt++) {
        uint32_t spi_freq = freq_list[attempt];
        Serial.printf("[SD] Testing %dMHz...\n", spi_freq / 1000000);
        LOG_INFO("SD", "Testing " + String(spi_freq / 1000000) + "MHz...");

        if(SD.begin(15, *sd_spi, spi_freq))  // SD-Card SS pin is 15
        {
            mounted = true;
            LOG_INFO("SD", "✓✓✓ SUCCESS! Card mounted at " + String(spi_freq / 1000000) + "MHz");
            Serial.printf("[SD] ✓✓✓ SUCCESS! Card mounted at %dMHz\n", spi_freq / 1000000);
            break;
        }

        // 失败后的完整复位流程
        if(attempt < freq_count - 1) {
            Serial.println("[SD] Failed, trying lower speed...");
            SD.end();  // 结束之前的尝试

            // 【完全复位】重新初始化SPI总线和SD卡
            sd_spi->end();  // 关闭SPI总线
            delay(100);

            digitalWrite(15, LOW);  // CS拉低
            delay(100);
            digitalWrite(15, HIGH);  // CS拉高
            delay(200);

            sd_spi->begin(14, 26, 13, 15);  // 重新初始化SPI
            delay(100);

            // 再次发送时钟脉冲
            sd_spi->beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));
            for(int i = 0; i < 10; i++) {
                sd_spi->transfer(0xFF);
            }
            sd_spi->endTransaction();
            delay(100);
        }
    }

    if(!mounted) {
        LOG_ERROR("SD", "Card Mount Failed at all speeds!");
        Serial.println("[SD] ✗✗✗ Card Mount Failed at all speeds!");
        return;
    }

    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE) {
        LOG_WARN("SD", "No SD card attached");
        return;
    }

    String cardTypeStr = "UNKNOWN";
    if(cardType == CARD_MMC) {
        cardTypeStr = "MMC";
    } else if(cardType == CARD_SD) {
        cardTypeStr = "SDSC";
    } else if(cardType == CARD_SDHC) {
        cardTypeStr = "SDHC";
    }

    LOG_INFO("SD", "SD Card Type: " + cardTypeStr);

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    LOG_INFO("SD", "SD Card Size: " + String(cardSize) + "MB");
}

void SdCard::listDir(const char* dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\n", dirname);

    File root = SD.open(dirname);
    if(!root) {
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()) {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file) {
        if(file.isDirectory()) {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels) {
                // 构建完整的子目录路径
                String subPath = String(dirname);
                if(!subPath.endsWith("/")) {
                    subPath += "/";
                }
                subPath += file.name();
                listDir(subPath.c_str(), levels - 1);
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

void SdCard::treeDir(const char* dirname, uint8_t levels, const char* prefix)
{
    File root = SD.open(dirname);
    if(!root) {
        Serial.printf("%s[Failed to open directory]\n", prefix);
        return;
    }
    if(!root.isDirectory()) {
        Serial.printf("%s%s [Not a directory]\n", prefix, dirname);
        return;
    }

    File file = root.openNextFile();
    while(file) {
        if(file.isDirectory()) {
            Serial.printf("%s[DIR]  %s/\n", prefix, file.name());
            if(levels > 0) {
                // 构建新的前缀字符串和子目录路径
                String newPrefix = String(prefix) + "|   ";
                String subPath = String(dirname);
                if(!subPath.endsWith("/")) {
                    subPath += "/";
                }
                subPath += file.name();
                treeDir(subPath.c_str(), levels - 1, newPrefix.c_str());
            }
        } else {
            // 格式化文件大小显示
            size_t fileSize = file.size();
            String sizeStr;
            if(fileSize < 1024) {
                sizeStr = String(fileSize) + "B";
            } else if(fileSize < 1024 * 1024) {
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

void SdCard::createDir(const char* path)
{
    Serial.printf("Creating Dir: %s\n", path);
    if(SD.mkdir(path)) {
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void SdCard::removeDir(const char* path)
{
    Serial.printf("Removing Dir: %s\n", path);
    if(SD.rmdir(path)) {
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}

void SdCard::readFile(const char* path)
{
    Serial.printf("Reading file: %s\n", path);

    File file = SD.open(path);
    if(!file) {
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while(file.available()) {
        Serial.write(file.read());
    }
    file.close();
}

String SdCard::readFileLine(const char* path, int num = 1)
{
    Serial.printf("Reading file: %s line: %d\n", path, num);

    File file = SD.open(path);
    if(!file) {
        return ("Failed to open file for reading");
    }

    char* p = buf;
    while(file.available()) {
        char c = file.read();
        if(c == '\n') {
            num--;
            if(num == 0) {
                *(p++) = '\0';
                String s(buf);
                s.trim();
                return s;
            }
        } else if(num == 1) {
            *(p++) = c;
        }
    }
    file.close();

    return String("error parameter!");
}

void SdCard::writeFile(const char* path, const char* message)
{
    Serial.printf("Writing file: %s\n", path);

    File file = SD.open(path, FILE_WRITE);
    if(!file) {
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)) {
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void SdCard::appendFile(const char* path, const char* message)
{
    Serial.printf("Appending to file: %s\n", path);

    File file = SD.open(path, FILE_APPEND);
    if(!file) {
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)) {
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

void SdCard::renameFile(const char* path1, const char* path2)
{
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if(SD.rename(path1, path2)) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}

void SdCard::deleteFile(const char* path)
{
    Serial.printf("Deleting file: %s\n", path);
    if(SD.remove(path)) {
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

void SdCard::readBinFromSd(const char* path, uint8_t* buf)
{
    File file = SD.open(path);
    size_t len = 0;
    if(file) {
        len = file.size();
        size_t flen = len;

        while(len) {
            size_t toRead = len;
            if(toRead > 512) {
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }

        file.close();
    } else {
        Serial.println("Failed to open file for reading");
    }
}

void SdCard::writeBinToSd(const char* path, uint8_t* buf)
{
    File file = SD.open(path, FILE_WRITE);
    if(!file) {
        Serial.println("Failed to open file for writing");
        return;
    }

    size_t i;
    for(i = 0; i < 2048; i++) {
        file.write(buf, 512);
    }
    file.close();
}

void SdCard::fileIO(const char* path)
{
    File file = SD.open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;
    if(file) {
        len = file.size();
        size_t flen = len;
        start = millis();
        while(len) {
            size_t toRead = len;
            if(toRead > 512) {
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }
        end = millis() - start;
        Serial.printf("%u bytes read for %u ms\n", flen, end);
        file.close();
    } else {
        Serial.println("Failed to open file for reading");
    }

    file = SD.open(path, FILE_WRITE);
    if(!file) {
        Serial.println("Failed to open file for writing");
        return;
    }

    size_t i;
    start = millis();
    for(i = 0; i < 2048; i++) {
        file.write(buf, 512);
    }
    end = millis() - start;
    Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
    file.close();
}
