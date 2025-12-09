# 串口文件传输功能使用指南

## 概述

CybirdWatching CLI v1.2.0 新增了通过串口传输文件到ESP32 SD卡的功能，无需频繁插拔SD卡即可更新配置、资源文件。

## 功能特性

- ✅ **文件上传** - 从PC上传文件到SD卡
- ✅ **文件下载** - 从SD卡下载文件到PC
- ✅ **文件删除** - 删除SD卡上的文件
- ✅ **文件信息** - 查看文件大小、类型等信息
- ✅ **Base64编码** - 安全可靠的二进制文件传输
- ✅ **进度显示** - 实时显示传输进度
- ✅ **断点续传保护** - 传输失败自动清理不完整文件

## 使用方法

### 1. 文件上传

#### 快捷方式（推荐）

```bash
[ON] CybirdWatching> upload <本地文件> <远程路径>
```

**示例：**

```bash
# 上传配置文件
upload ./bird_config.json /configs/bird_config.json

# 上传小鸟资源
upload ./1.bin /birds/1001/1.bin

# 上传到新目录（自动创建）
upload ./logo.bin /static/logo.bin
```

#### 完整协议

```bash
# 1. 发送上传命令
file upload /configs/bird_config.json

# 2. 设备响应 READY，发送文件大小
FILE_SIZE:957

# 3. 发送Base64编码的数据
QXJyYXlPZkJhc2U2NEVuY29kZWREYXRhLi4u

# 4. 发送结束标记
FILE_END

# 5. 设备确认
SUCCESS: File uploaded successfully!
```

### 2. 文件下载

#### 快捷方式（推荐）

```bash
[ON] CybirdWatching> download <远程路径> <本地文件>
```

**示例：**

```bash
# 下载配置文件
download /configs/bird_config.json ./downloaded_config.json

# 下载日志文件
download /logs/cybird_watching.log ./device.log

# 下载小鸟资源
download /birds/1001/1.bin ./bird_frame.bin
```

### 3. 文件信息

```bash
file info /configs/bird_config.json
```

**输出示例：**

```
=== File Information ===
Path: /configs/bird_config.json
Size: 957 bytes (0.93 KB)
Type: File
========================
```

### 4. 文件删除

```bash
file delete /temp/old_file.txt
```

**警告：** 删除操作不可逆，请谨慎使用！

## 实际应用场景

### 场景1：更新配置文件

修复了bird_config.json中的ID重复问题后，上传新配置：

```bash
[ON] CybirdWatching> upload ./resources/configs/bird_config.json /configs/bird_config.json
准备上传文件: ./resources/configs/bird_config.json
目标路径: /configs/bird_config.json
文件大小: 957 字节 (0.93 KB)
设备已就绪，开始传输...
进度: 768/957 字节 (80.3%)
进度: 957/957 字节 (100.0%)
数据传输完成，等待设备确认...
设备响应: SUCCESS: File uploaded successfully!
✓ 文件上传成功!
```

### 场景2：批量上传小鸟资源

```bash
# Windows批处理脚本
@echo off
for %%f in (resources\birds\1013\*.bin) do (
    echo Uploading %%f...
    cybird-cli send "file upload /birds/1013/%%~nxf"
    timeout /t 2
)

# Linux/Mac bash脚本
#!/bin/bash
for file in resources/birds/1013/*.bin; do
    filename=$(basename "$file")
    echo "Uploading $filename..."
    cybird-cli --port /dev/ttyUSB0 send "file upload /birds/1013/$filename"
    sleep 2
done
```

### 场景3：备份设备数据

```bash
# 下载所有统计数据
download /stats/bird_stats.json ./backup/bird_stats.json

# 下载配置
download /configs/bird_config.json ./backup/bird_config.json

# 下载日志
download /logs/cybird_watching.log ./backup/device.log
```

### 场景4：远程调试

```bash
# 查看设备文件结构
tree /birds 2

# 检查特定文件
file info /birds/1013/1.bin

# 下载问题文件进行本地分析
download /birds/1013/81.bin ./debug/problematic_file.bin

# 上传修复后的文件
upload ./debug/fixed_file.bin /birds/1013/81.bin
```

## 传输协议细节

### Base64编码

- **编码块大小**: 768字节 → 1024字符（Base64）
- **行分隔**: 每行一个编码块
- **字符集**: `A-Za-z0-9+/=`

### 超时设置

| 阶段 | 超时时间 | 说明 |
|------|----------|------|
| READY等待 | 10秒 | 等待设备准备接收 |
| 文件大小确认 | 30秒 | 等待FILE_SIZE响应 |
| 数据传输 | 2分钟 | 每次接收数据重置 |
| 成功确认 | 30秒 | 等待SUCCESS响应 |

### 进度显示

```
进度: <已传输字节> / <总字节> (百分比%)
```

示例：
```
进度: 7680/12480 字节 (61.5%)
```

## 性能优化建议

### 1. 传输大文件

对于大文件（>100KB），建议：

```python
# 增大缓冲区
file_transfer.chunk_size = 1536  # 加倍

# 减少延迟
await asyncio.sleep(0.001)  # 从0.01改为0.001
```

### 2. 批量传输

使用脚本自动化：

```python
import asyncio
from cybird_watching_cli.core.file_transfer import FileTransfer

async def batch_upload(files):
    for local, remote in files:
        await file_transfer.upload_file(local, remote)
        print(f"✓ {remote} uploaded")
```

### 3. 错误重试

```python
max_retries = 3
for attempt in range(max_retries):
    try:
        await file_transfer.upload_file(local, remote)
        break
    except FileTransferError as e:
        if attempt == max_retries - 1:
            raise
        print(f"Retry {attempt + 1}/{max_retries}...")
        await asyncio.sleep(2)
```

## 故障排除

### 问题1：传输超时

**症状：**
```
错误: 等待设备READY信号超时
```

**解决方法：**
1. 检查设备固件版本（是否支持file命令）
2. 确认设备未处于繁忙状态
3. 增大超时时间
4. 检查串口缓冲区设置

### 问题2：文件不完整

**症状：**
```
错误: Transfer timeout or incomplete
```

**解决方法：**
1. 检查串口连接稳定性
2. 降低传输速度（减小chunk_size）
3. 检查设备内存是否充足
4. 尝试重新传输

### 问题3：SD卡权限错误

**症状：**
```
ERROR: /sd/birds/1013/81.bin does not exist, no permits for creation
```

**解决方法：**
1. 确认SD卡已正确挂载
2. 先创建目录：`mkdir /birds/1013`
3. 检查文件系统健康状态
4. 重新格式化SD卡（如必要）

### 问题4：Base64解码失败

**症状：**
```
警告: 解码数据失败
```

**解决方法：**
1. 检查数据完整性
2. 确认没有控制字符混入
3. 重新传输
4. 检查串口设置（无奇偶校验，8数据位，1停止位）

## 安全注意事项

### 1. 路径安全

- ✅ 使用绝对路径（以`/`开头）
- ❌ 避免使用`..`等相对路径
- ✅ 检查路径长度（不超过128字符）

### 2. 文件大小限制

- 单个文件建议 < 1MB
- SD卡剩余空间检查
- 避免传输过大文件导致超时

### 3. 数据验证

```python
# 上传后验证
file_info = await file_transfer.get_file_info(remote_path)
if file_info and file_info.get('Size'):
    size = int(file_info['Size'].split()[0])
    if size == local_file_size:
        print("✓ 文件大小验证通过")
```

## 常用命令速查

| 操作 | 命令 |
|------|------|
| 上传文件 | `upload <本地> <远程>` |
| 下载文件 | `download <远程> <本地>` |
| 文件信息 | `file info <远程>` |
| 删除文件 | `file delete <远程>` |
| 查看目录 | `tree <路径> <层级>` |
| 帮助信息 | `file help` |

## 更新日志

### v1.2.0 (2025-12-04)
- ✨ 首次发布文件传输功能
- ✨ 支持上传/下载/删除/信息查询
- ✨ Base64编码确保二进制文件安全传输
- ✨ 实时进度显示
- 🐛 修复bird_config.json ID重复问题

## 相关文档

- [主README](../scripts/cybird_watching_cli/README.md)
- [ESP32固件串口命令文档](./serial_commands.md)
- [配置文件格式说明](./configuration.md)
