# 添加新小鸟指南

本指南详细介绍如何向 Cybird Watching 项目中添加新的小鸟物种。

---

## 📋 目录

1. [准备工作](#准备工作)
2. [步骤 1: 小鸟资源转换](#步骤-1-小鸟资源转换)
3. [步骤 2: 添加配置](#步骤-2-添加配置)
4. [步骤 3: 采集全量文字](#步骤-3-采集全量文字)
5. [步骤 4: 新增字体](#步骤-4-新增字体)
6. [步骤 5: 重新烧录并测试](#步骤-5-重新烧录并测试)
7. [常见问题](#常见问题)

---

## 准备工作

在开始之前，请确保已安装以下工具：

- ✅ Python 3.x 和 `uv` 包管理器
- ✅ PlatformIO 开发环境
- ✅ 小鸟的视频资源（.mp4 格式）或图片序列

**资源规格要求**：
- 视频帧率：按 15 FPS采样
- 输出尺寸：120x120 像素
- 输出格式：RGB565 格式的 .bin 文件

也可以自行调整脚本修改规格，但是超出上述规格的资源大概率会导致 core dump。

---

## 步骤 1: 小鸟资源转换

根据你拥有的资源类型，选择对应的转换方式。

### 方式 A: 批量转换 MP4 视频文件

**适用场景**：你有一个或多个小鸟的 .mp4 视频文件（建议1:1的视频比例， 分辨率 <= 1080p）

1. **运行批量转换脚本**：
   ```bash
   cd scripts
   batch_convert_mp4.bat
   ```

2. **按提示输入信息**：
   ```
   Enter source directory path: E:\bird_videos\
   Enter target directory path: E:\Projects\cybird-watching\resources\birds\
   Confirm conversion? (Y/N): Y
   ```

3. **转换参数**（自动配置）：
   - 输出格式：`rgb565`
   - 帧率：`15 FPS`
   - 尺寸：`120x120`
   - 回文模式：启用（palindrome，正向播放后反向播放，实现循环效果）
   - 并发数：`2 workers`

4. **输出文件位置**：
   - 生成的 .bin 文件会保存到你指定的目标目录
   - 建议放置路径：`resources/birds/<bird_id>/`

### 方式 B: 转换图片序列

**适用场景**：你有按顺序命名的图片序列（如 frame001.png, frame002.png...）

1. **准备图片序列**：
   - 确保图片按顺序命名，而且比例为 1:1
   - 图片尺寸建议为 120x120 或更大（会自动缩放）

2. **运行图片转换脚本**：
   ```bash
   cd scripts
   run_convert.bat
   ```

3. **按提示输入信息**：
   ```
   Enter source directory path: E:\bird_images\kingfisher\
   Enter target directory path: E:\Projects\cybird-watching\resources\birds\1001\
   Confirm conversion? (Y/N): Y
   ```

4. **脚本执行流程**：
   - 自动同步依赖包
   - 将图片序列转换为单个 .bin 文件

### 资源文件命名规范

按以下结构组织资源：

```
resources/
└── birds/
    ├── 1001/            # 小鸟 ID
    │   └── bundle.bin   # 动画资源文件（单一文件）
    ├── 1002/
    │   └── bundle.bin
    └── ...
```

---

## 步骤 2: 添加配置

编辑小鸟配置文件，添加新的小鸟信息。

### 编辑配置文件

打开配置文件：`resources/configs/bird_config.csv`

### 配置格式说明

```csv
id, name, weight
1054,新小鸟名称,50
```

**字段说明**：

| 字段 | 说明 | 示例 |
|------|------|------|
| `id` | 小鸟唯一标识符，4位数字，接着之前的最后一个配置递增 | `1054` |
| `name` | 小鸟的中文名称 | `蓝喉蜂虎` |
| `weight` | 出现权重（1-100），数值越大出现概率越高 | `50` |

### 权重设置建议

根据小鸟的稀有程度设置权重：

- **极常见**：`100`（如白头鹎、珠颈斑鸠）
- **常见**：`60-90`（如大山雀、鹊鸲）
- **普通**：`40-50`（如普通翠鸟、白胸翡翠）
- **较少见**：`20-30`（如冠鱼狗、斑鱼狗）
- **稀有**：`10`（如各种猛禽）
- **极稀有**：`5`（如鹰雕、红隼）

### 配置示例

```csv
id, name, weight
1001,普通翠鸟,50
1002,白胸翡翠,50
1003,冠鱼狗,20
...
1053,褐冠鹃隼,10
1054,蓝喉蜂虎,30
1055,棕背伯劳,40
```

**注意事项**：
- ⚠️ ID 不要重复
- ⚠️ 确保 CSV 格式正确（逗号分隔，无多余空格）
- ⚠️ 名称必须使用标准的中文鸟名

---

## 步骤 3: 采集全量文字

当添加新的小鸟名称后，需要重新采集全部文字，用于生成字体文件。

### 运行采集脚本

```bash
cd scripts/uniq_fonts
python main.py
```

### 脚本功能

脚本会：
1. 从 `bird_config.csv` 和其他配置中提取所有中文字符
2. 去重并排序
3. 输出完整的字符集

### 输出示例

```
注意第一个空格字符也要拷贝
 !#$%&()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]^_abcdefghijklmnopqrstuvwxyz一上下...
```

### 复制字符集

1. **完整复制输出的字符串**（包括开头的空格）
2. **保存到剪贴板**，将在下一步使用

**⚠️ 重要提示**：
- 必须包含第一个空格字符
- 不要遗漏任何字符，否则字体生成后可能显示为方块（□）

---

## 步骤 4: 新增字体

使用采集到的字符集生成新的字体文件。

### 访问字体生成工具

打开 [LVGL Font Converter](https://lvgl.io/tools/fontconverter)

### 配置字体参数

根据当前项目配置，填写以下参数：

| 参数 | 值 | 说明 |
|------|------|------|
| **Name** | `notosanssc_16` | 字体名称（不含 lv_font_ 前缀） |
| **Size** | `16` | 当前项目使用 16px |
| **Bpp** | `2 bit-per-pixel` | 推荐值，平衡质量和空间 |
| **Font** | `NotoSansSC-Medium.ttf` | 思源黑体（备份在 `scripts/uniq_fonts/fonts/`） |
| **Format** | `Binary` | ⚠️ 必须选择 Binary 格式！ |
| **Symbols** | 见下方 | 使用步骤 3 生成的字符集 |

### 配置 Symbols（字符范围）

**不要使用** Unicode Range 方式（会生成过大的字体文件）

选择 **Symbols** 方式：

1. 选择 `Symbols` 单选框
2. 将步骤 3 生成的 `scripts/uniq_fonts/font_chars.txt` 内容粘贴到文本框
3. 确保包含所有字符（包括空格）

### 生成和下载字体

1. 点击 `Convert` 按钮
2. 等待生成完成
3. 下载生成的 `.bin` 文件
4. **重命名**为 `.fnt` 格式：`notosanssc_16.bin` → `notosanssc_16.fnt`

### 放置字体文件

将字体文件复制到 **SD 卡**：

```
SD卡根目录/
└── fonts/
    ├── notosanssc_12.fnt
    ├── notosanssc_16.fnt  ← 替换或新增
    └── notosanssc_18.fnt
```

**⚠️ 注意**：字体文件存放在 SD 卡，不是固件中！

### 可选：其他字体大小

如果需要调整字体大小，请参考：[修改字体大小指南](CHANGE_FONT_SIZE.md)

支持的字体大小：12px、14px、**16px**（当前）、18px

---

## 步骤 5: 重新烧录并测试

完成上述配置后，重新编译并上传到设备。

### 编译和上传

```bash
platformio run --target upload
```

或使用快捷脚本：

```bash
cd scripts
pio_run.bat
```

### 准备 SD 卡

确保 SD 卡包含以下内容：

```
SD卡根目录/
├── fonts/
│   └── notosanssc_16.fnt  ← 字体文件
├── configs/
│   └── bird_config.csv    ← 小鸟配置
└── birds/
    └── <id>/              ← 小鸟动画资源
        └── *.bin
```

### 监控串口输出

```bash
platformio device monitor
```

或使用：

```bash
cd scripts
upload_and_monitor.bat
```

### 测试检查项

在设备上测试以下功能：

- ✅ **字体加载**：检查串口日志是否显示 `[INFO] FONT: Font loaded: notosanssc_16`
- ✅ **新小鸟显示**：检查新添加的小鸟是否正确显示（用 `bird trigger` 串口命令直接触发）
- ✅ **动画播放**：确认动画流畅播放
- ✅ **名称显示**：检查小鸟名称是否正常显示（无方块或乱码）
- ✅ **权重生效**：观察新小鸟是否按预期权重出现
- ✅ **统计功能**：进入统计页面，确认新小鸟被正确统计

### 调试建议

如果遇到问题：

1. **小鸟不出现**：
   - 检查 `bird_config.csv` 格式是否正确
   - 检查 ID 是否重复
   - 权重是否设置过低

2. **名称显示为方块（□）**：
   - 重新运行步骤 3 采集文字
   - 确认步骤 4 字体生成时包含了所有字符
   - 检查字体文件是否正确放置到 SD 卡 `/fonts/` 目录
   - 检查串口日志是否有字体加载错误

3. **字体加载失败**：
   - 确认字体文件扩展名是 `.fnt`（不是 `.bin`）
   - 确认在线工具选择了 **Binary** 格式
   - 检查 SD 卡是否正确挂载

4. **动画不显示**：
   - 检查 .bin 文件路径是否正确
   - 检查文件命名是否符合规范（`resources/birds/<id>/bundle.bin`）

---

## 常见问题

### Q1: 如何批量添加多个小鸟？

**A**: 依次完成每个小鸟的资源转换，然后在 `bird_config.csv` 中一次性添加所有配置，最后统一生成字体。

### Q2: 视频资源太大怎么办？

**A**: 
- 降低帧率（如 10 FPS）
- 减少视频时长（截取关键片段）
- 使用 palindrome 模式（回文播放）减少帧数

### Q3: 字体文件生成失败？

**A**: 
- 确保字符集中没有特殊控制字符
- 尝试使用较小的字体大小
- 检查字符数量是否过多（建议 < 3000 个汉字）
- 确认选择了 **Binary** 格式（不是 C Array）

### Q4: 设备无法启动或卡顿？

**A**: 
- 检查 SD 卡是否正确格式化（FAT32）
- 确认字体文件路径正确（`/fonts/notosanssc_16.fnt`）
- 减少动画资源大小

### Q5: 如何删除已添加的小鸟？

**A**: 
1. 从 `bird_config.csv` 中删除对应行
2. 删除对应的资源文件（`resources/birds/<id>/`）
3. 重新采集文字并生成字体（如果删除的小鸟名称包含独有汉字）
4. 重新编译上传

### Q6: 转换工具报错？

**A**: 
- 确保已安装 `uv` 包管理器：`pip install uv`
- 检查 Python 版本（建议 3.8+）
- 进入对应工具目录手动执行：`cd scripts/mp4converter && uv sync`

---

## 快速参考

### 添加新小鸟检查清单

- [ ] 准备视频/图片资源
- [ ] 运行转换脚本生成 .bin 文件
- [ ] 将 .bin 文件放到 `resources/birds/<id>/` 目录
- [ ] 编辑 `bird_config.csv` 添加配置
- [ ] 运行 `uniq_fonts/main.py` 采集文字
- [ ] 访问 LVGL Font Converter 生成字体（选择 **Binary** 格式）
- [ ] 将 `.bin` 重命名为 `.fnt`，复制到 SD 卡 `/fonts/` 目录
- [ ] 编译并上传固件
- [ ] 插入 SD 卡，在设备上测试验证

### 相关文件路径

| 文件 | 路径 |
|------|------|
| 小鸟配置 | `resources/configs/bird_config.csv` |
| 小鸟资源 | `resources/birds/<id>/bundle.bin` |
| 字体文件 | SD 卡 `/fonts/*.fnt` |
| 转换脚本 | `scripts/batch_convert_mp4.bat` |
| 图片转换 | `scripts/run_convert.bat` |
| 文字采集 | `scripts/uniq_fonts/main.py` |
| 字体生成指南 | `resources/fonts/README.md` |

### 有用的命令

```bash
# 编译上传
platformio run --target upload

# 监控串口
platformio device monitor

# 清理构建
platformio run --target clean

# 检查 Flash 使用
platformio run --target size
```

---

## 参考文档

- [修改字体大小指南](CHANGE_FONT_SIZE.md) - 字体配置详解
- [字体生成指南](../resources/fonts/README.md) - binfont 格式字体生成
- [CLI 工具使用说明](../scripts/README_CLI_TOOLS.md) - 命令行工具文档
- [LVGL 字体转换器](https://lvgl.io/tools/fontconverter) - 在线字体生成工具

---

**祝你成功添加新的小鸟！如有问题，请参考上述文档或查看项目 Issues。** 🐦
