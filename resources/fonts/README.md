# 字体文件生成指南

本目录用于存放 LVGL binfont 格式的字体文件。

## 文件列表

需要生成以下字体文件：

| 字体名称 | 字号 | 文件名 | 大小（估计） |
|---------|------|--------|-------------|
| notosanssc | 12px | `notosanssc_12.fnt` | ~30KB |
| notosanssc | 16px | `notosanssc_16.fnt` | ~40KB |
| notosanssc | 18px | `notosanssc_18.fnt` | ~45KB |

## 生成步骤

### 1. 生成 Symbols 字符集

运行脚本自动提取所有需要的字符：

```bash
cd scripts/uniq_fonts
python main.py
```

脚本会从以下文件提取字符：
- `src/config/ui_texts.h` - UI 文本
- `resources/configs/bird_config.csv` - 小鸟名称

生成的字符集保存在 `scripts/uniq_fonts/font_chars.txt`

### 2. 访问在线工具

https://lvgl.io/tools/fontconverter

### 3. 配置参数

**所有字体的共同配置**：

```
TTF/OTF文件:  NotoSansSC-Medium.ttf (备份在 scripts/uniq_fonts/fonts/)
Bpp:          2 bit-per-pixel
Format:       Binary ⚠️ 必须选择 Binary!
Symbols:      复制 scripts/uniq_fonts/font_chars.txt 的内容
```

### 4. 各字体具体配置

#### notosanssc_12.fnt

```
Name:    notosanssc_12
Size:    12 px
Symbols: (从 font_chars.txt 复制)
```

#### notosanssc_16.fnt

```
Name:    notosanssc_16
Size:    16 px
Symbols: (从 font_chars.txt 复制)
```

#### notosanssc_18.fnt

```
Name:    notosanssc_18
Size:    18 px
Symbols: (从 font_chars.txt 复制)
```

**当前 Symbols 参考**（可能会更新，请以脚本生成为准）：
```
!#$%&()*+,-./0123456789:@ABCDEFGHIJKLMNOPQRSTUVWXYZ^_abcdefghijklmnopqrstuvwxyz—…。䴘䴙一上下乌了八冠凤加卷叉反发叶哥啄喜嘴噪夜大太头寿小尾山带恶戴拟捣文斑新日普暗木本来松林次水池灰燕狗珠琵白眶眼紫红统绣绿缝翅翠翡耳胜胸脚脸腰腹苇苍苦莺蓝蛇蛋蜂褐观角计通金银长阳隼雀雕雨页领颈鬼鱼鳽鵟鸟鸠鸢鸦鸫鸬鸮鸰鸲鸺鸻鹃鹊鹎鹚鹞鹠鹡鹬鹭鹰黄黑！（），：？￥
```

### 4. 下载和重命名

- 点击 "Convert" 生成字体
- 下载生成的文件
- 如果下载的是 `.bin`，重命名为 `.fnt`：
  ```bash
  mv notosanssc_12.bin notosanssc_12.fnt
  mv notosanssc_16.bin notosanssc_16.fnt
  mv notosanssc_18.bin notosanssc_18.fnt
  ```

### 5. 复制到 SD 卡

将生成的 `.fnt` 文件复制到 SD 卡的 `/fonts/` 目录：

```
SD卡根目录/
└── fonts/
    ├── notosanssc_12.fnt
    ├── notosanssc_16.fnt
    └── notosanssc_18.fnt
```

## 使用方法

在代码中：

```cpp
#include "applications/modules/resources/fonts/font_manager.h"

// 加载字体
lv_font_t* font = FONT_MANAGER.loadFont("notosanssc_16");

// 应用到 LVGL 对象
if (font) {
    lv_obj_set_style_text_font(label, font, 0);
}
```

## 验证字体

插入 SD 卡后启动设备，查看串口日志：

```
[INFO] FONT: Loading font from: S:/fonts/notosanssc_16.fnt
[INFO] FONT: Font loaded successfully: notosanssc_16 (total: 1)
```

## 故障排查

### 加载失败

1. 检查 SD 卡是否正确挂载
2. 确认文件名拼写正确（区分大小写）
3. 确认文件扩展名是 `.fnt` 不是 `.bin`
4. 确认在线工具选择了 "Binary" 格式

### 显示乱码

- 字体文件中可能不包含该字符
- 重新生成时检查 Symbols 参数是否完整

## 注意事项

- ⚠️ 必须使用 **Binary** 格式，不是 C Array 格式
- ⚠️ Symbols 参数必须完整复制，包含所有字符
- ⚠️ 文件名必须与代码中 loadFont() 的参数一致

## 参考

- LVGL 字体工具：https://lvgl.io/tools/fontconverter
- Symbols 生成脚本：`scripts/uniq_fonts/main.py`
- 详细文档：`docs/CHANGE_FONT_SIZE.md`
