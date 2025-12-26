# 修改字体大小指南

## 当前架构

字体现在从 **SD 卡动态加载**（LVGL binfont 格式），不再编译到固件中。

```
SD卡根目录/
└── fonts/
    ├── notosanssc_12.fnt
    ├── notosanssc_16.fnt
    └── notosanssc_18.fnt
```

**优点**：
- 节省固件 Flash 空间（~281KB）
- 运行时动态切换字体
- 添加新字体无需重新编译

---

## 如何修改字体大小

### 步骤 1: 生成字体文件

访问: https://lvgl.io/tools/fontconverter

配置参数（以 18px 为例）:
```
Name:     notosanssc_18
Size:     18 px
Bpp:      2 bit-per-pixel
TTF/OTF:  NotoSansSC-Medium.ttf  # 备份在 scripts/uniq_fonts/fonts/
Format:   Binary ⚠️ 必须选择 Binary!
Symbols:  见 resources/fonts/README.md
```

### 步骤 2: 放入 SD 卡

将生成的 `.bin` 文件重命名为 `.fnt`，复制到 SD 卡：

```bash
# 重命名
mv notosanssc_18.bin notosanssc_18.fnt

# 复制到 SD 卡
cp notosanssc_18.fnt /path/to/sdcard/fonts/
```

### 步骤 3: 代码中加载字体

```cpp
#include "applications/modules/resources/fonts/font_manager.h"

// 加载字体
lv_font_t* font = FONT_MANAGER.loadFont("notosanssc_18");

// 应用到 LVGL 对象
if (font) {
    lv_obj_set_style_text_font(label, font, 0);
}
```

无需重新编译固件！

---

## 支持的字体大小

| 大小 | 字体文件 | 文件大小（估计） |
|------|---------|-----------------|
| 12px | `notosanssc_12.fnt` | ~30KB |
| **16px** | `notosanssc_16.fnt` ✅ | ~40KB |
| 18px | `notosanssc_18.fnt` | ~45KB |

**注意**：字体文件存储在 SD 卡，不占用固件 Flash 空间。

---

## FontManager API

```cpp
// 获取单例
FontManager& fm = FontManager::getInstance();
// 或使用宏
FONT_MANAGER

// 加载字体（自动缓存，重复调用返回已加载的字体）
lv_font_t* font = FONT_MANAGER.loadFont("notosanssc_16");

// 检查字体是否已加载
bool loaded = FONT_MANAGER.isFontLoaded("notosanssc_16");

// 获取已加载的字体
lv_font_t* font = FONT_MANAGER.getFont("notosanssc_16");

// 销毁字体（释放内存）
FONT_MANAGER.destroyFont("notosanssc_16");
FONT_MANAGER.destroyFont(font);  // 或传入指针

// 销毁所有字体
FONT_MANAGER.destroyAllFonts();
```

---

## 文件系统架构

字体通过 LVGL 文件系统驱动访问 SD 卡：

```
LVGL 路径: S:/fonts/notosanssc_16.fnt
           │
           └─ 'S' 是 LVGL 文件系统驱动器字母
              映射到 SD 卡根目录
```

相关文件：
- `src/system/lvgl/ports/lv_port_fatfs.cpp` - LVGL 9.x 文件系统驱动
- `src/drivers/display/display.cpp` - 初始化时调用 `lv_fs_if_init()`

---

## 快速切换字体大小

### 示例：从 16px 改为 18px

1. **确保字体文件存在**：检查 SD 卡 `/fonts/notosanssc_18.fnt`

2. **修改代码**：
```cpp
// 改为加载 18px 字体
lv_font_t* font = FONT_MANAGER.loadFont("notosanssc_18");
```

3. **无需重新编译**（如果只是换字体文件）

---

## 常见问题

### Q: 字体加载失败？

检查串口日志：
```
[ERROR] FONT: Failed to load font: S:/fonts/notosanssc_18.fnt
```

排查步骤：
1. 确认 SD 卡已正确挂载
2. 确认文件名正确（区分大小写）
3. 确认扩展名是 `.fnt` 不是 `.bin`
4. 确认在线工具选择了 **Binary** 格式

### Q: 显示乱码？

字体文件中可能不包含该字符。重新生成时检查 Symbols 参数是否完整。

### Q: 能否同时使用多个字体大小？

可以！FontManager 支持同时加载多个字体：
```cpp
lv_font_t* font16 = FONT_MANAGER.loadFont("notosanssc_16");
lv_font_t* font18 = FONT_MANAGER.loadFont("notosanssc_18");
```

### Q: 内存占用多少？

LVGL 自动管理字形缓存，运行时内存占用约 20-50KB。

---

## 最佳实践

**推荐配置**（平衡清晰度和文件大小）:
```
Size: 16px
Bpp:  2 bit-per-pixel
```

**小屏幕优化**:
```
Size: 14px
Bpp:  2 bit-per-pixel
```

**清晰优先**:
```
Size: 18px
Bpp:  4 bit-per-pixel
```

---

## 参考文档

- 字体生成详细指南：`resources/fonts/README.md`
- LVGL 字体工具：https://lvgl.io/tools/fontconverter
