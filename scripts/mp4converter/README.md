# MP4转RGB565转换器

将MP4视频转换为LVGL 7.9.1兼容的RGB565格式图像帧序列的专业工具，专为CybirdWatching项目设计。

## 🆕 新功能

- **双格式输出**: 支持RGB565(.bin)和PNG格式输出
- **自动抠图**: 智能去除绿底，生成透明背景图片
- **统一命名**: 文件名统一从1递增，格式为`视频名_0001.bin`或`视频名_0001.png`

## 功能特性

- **智能帧采样**: 支持按帧率或帧数采样，均匀分布算法
- **图像处理**: 智能缩放、尺寸调整、质量优化
- **水印去除**: 基础区域填充，去除指定区域水印
- **自动抠图**: 智能检测绿幕并去除，支持边缘羽化
- **批量处理**: 多线程并行处理，进度跟踪
- **完全兼容**: 与LVGL 7.9.1和现有converter无缝集成

## 安装

```bash
uv sync
```

## 快速开始

### 基本用法

#### 1. 处理单个视频 - RGB565格式（默认）
```bash
# 基础转换 - 使用原始帧率
mp4-converter process video.mp4 output_frames/

# 指定帧率采样 - 每秒提取10帧
mp4-converter process video.mp4 output_frames/ --frame-rate 10

# 缩放分辨率到120x120
mp4-converter process video.mp4 output_frames/ --resize 120x120

# 去除左上角水印
mp4-converter process video.mp4 output_frames/ --watermark-region "0,0,50,50"
```

#### 2. 🆕 输出PNG格式
```bash
# 输出PNG格式，不转换为RGB565
mp4-converter process video.mp4 output_frames/ --output-format png

# PNG格式 + 自动抠图（去除绿底）
mp4-converter process video.mp4 output_frames/ --output-format png --chroma-key

# PNG格式 + 缩放 + 抠图 + 去水印
mp4-converter process green_screen.mp4 output/ \
    --output-format png \
    --chroma-key \
    --resize 200x200 \
    --watermark-region "10,10,100,30"
```

#### 3. 🆕 批量处理
```bash
# 批量处理，输出PNG格式
mp4-converter batch videos/ output/ --output-format png --workers 8

# 批量处理 + 自动抠图
mp4-converter batch green_videos/ transparent_output/ \
    --output-format png \
    --chroma-key \
    --frame-rate 5 \
    --workers 4

# 混合处理，不同视频不同配置
mp4-converter batch mixed_videos/ output/ \
    --frame-rate 10 \
    --resize 120x120 \
    --chroma-key \
    --continue-on-error
```

#### 4. 查看视频信息
```bash
mp4-converter info video.mp4
```

## 🆕 新功能详解

### 输出格式选择

```bash
# RGB565格式（默认）- 用于嵌入式显示
mp4-converter process video.mp4 rgb565_output/

# PNG格式 - 用于一般图片处理
mp4-converter process video.mp4 png_output/ --output-format png
```

**输出文件命名:**
- RGB565: `视频名_0001.bin`, `视频名_0002.bin`...
- PNG: `视频名_0001.png`, `视频名_0002.png`...

### 自动抠图功能

```bash
# 自动检测绿幕并去除
mp4-converter process greenscreen.mp4 output/ --chroma-key

# 适用于绿幕拍摄的素材
mp4-converter batch green_screen_videos/ transparent_frames/ \
    --output-format png \
    --chroma-key
```

**抠图特性:**
- 智能检测绿色幕布
- 边缘羽化处理
- 透明背景输出（PNG格式）
- 自动优化边缘效果

### 完整功能示例

```bash
# 处理绿幕视频，输出透明背景PNG
mp4-converter process greenscreen_demo.mp4 demo_frames/ \
    --output-format png \
    --chroma-key \
    --frame-rate 15 \
    --resize 300x300

# 处理普通视频，输出RGB565用于嵌入式
mp4-converter process product_video.mp4 embedded_frames/ \
    --output-format rgb565 \
    --rgb565-format binary \
    --frame-count 30 \
    --resize 120x120

# 批量处理混合素材
mp4-converter batch mixed_videos/ final_output/ \
    --output-format png \
    --chroma-key \
    --frame-rate 8 \
    --resize 200x200 \
    --workers 6 \
    --continue-on-error
```