# 修改字体大小指南

## 当前配置

```c
#define BIRD_INFO_FONT_SIZE 16  // 当前使用 16px
```

字体文件: `lv_font_notosanssc_16.c`

---

## 如何修改字体大小

### 步骤 1: 修改配置

编辑 `src/applications/gui/screens/setup_scr_scenes.c`:

```c
#define BIRD_INFO_FONT_SIZE 18  // 改为你想要的大小（12/14/16/18/20）
```

### 步骤 2: 生成对应大小的字体

访问: https://lvgl.io/tools/fontconverter

配置参数（以 18px 为例）:
```
Name:  lv_font_notosanssc_18  ⚠️ 数字要匹配
Size:  18
Bpp:   2 bit-per-pixel
Font:  NotoSansSC-Regular.ttf
Range: 0x20-0x7F, 0x4E00-0x9FFF
```

### 步骤 3: 更新字体声明

编辑 `src/config/guider_fonts.h`，添加新的字体声明:

```c
LV_FONT_DECLARE(lv_font_notosanssc_16)  // 保留旧的
LV_FONT_DECLARE(lv_font_notosanssc_18)  // 添加新的
```

### 步骤 4: 放入字体文件

将生成的 `lv_font_notosanssc_18.c` 放到:
```
src/applications/modules/resources/fonts/
```

### 步骤 5: 编译上传

```bash
platformio run --target upload
```

---

## 支持的字体大小

代码已配置支持以下大小：

| 大小 | 中文字体 | 英文字体 | 文件大小估计 |
|------|---------|---------|-------------|
| 12px | `lv_font_notosanssc_12` | `lv_font_montserrat_12` | ~350KB |
| 14px | `lv_font_notosanssc_14` | `lv_font_montserrat_14` | ~450KB |
| **16px** | `lv_font_notosanssc_16` ✅ | `lv_font_montserrat_16` | ~500KB |
| 18px | `lv_font_notosanssc_18` | `lv_font_montserrat_18` | ~600KB |
| 20px | `lv_font_notosanssc_20` | `lv_font_montserrat_20` | ~700KB |

---

## 当前 Flash 使用情况

```
总容量: 1,310,720 bytes
已使用:   748,225 bytes (57.1%)
剩余:     562,495 bytes (42.9%)

添加 16px 字体后:
预计使用: 1,248,225 bytes (95.2%)
剩余:        62,495 bytes (4.8%)
```

**建议**: 
- ✅ 16px 及以下 - 空间充足
- ⚠️ 18px - 空间紧张，可能需要清理
- ❌ 20px - 可能超出 Flash 容量

---

## 快速切换字体大小

### 示例：从 16px 改为 14px

1. **修改配置**:
```c
#define BIRD_INFO_FONT_SIZE 14  // 从 16 改为 14
```

2. **生成字体**: `lv_font_notosanssc_14.c`

3. **更新声明**:
```c
LV_FONT_DECLARE(lv_font_notosanssc_14)
```

4. **编译**: 代码会自动使用 `lv_font_notosanssc_14`

---

## 代码逻辑

配置会自动选择对应的字体：

```c
#if BIRD_INFO_FONT_SIZE == 16
    #define BIRD_INFO_FONT lv_font_notosanssc_16
#elif BIRD_INFO_FONT_SIZE == 18
    #define BIRD_INFO_FONT lv_font_notosanssc_18
// ... 其他大小
#endif

// 使用时
lv_obj_set_style_text_font(label, &BIRD_INFO_FONT, LV_PART_MAIN);
```

**优点**:
- ✅ 修改一个数字即可切换字体大小
- ✅ 自动选择对应的字体文件
- ✅ 编译时检查，避免错误

---

## 常见问题

### Q: 修改 BIRD_INFO_FONT_SIZE 后编译报错？

A: 你需要先生成对应大小的字体文件并添加声明。例如改为 18，需要：
1. 生成 `lv_font_notosanssc_18.c`
2. 添加 `LV_FONT_DECLARE(lv_font_notosanssc_18)`
3. 放置字体文件到 fonts 目录

### Q: 能否同时保留多个字体大小？

A: 可以！生成多个字体文件，代码会根据 `BIRD_INFO_FONT_SIZE` 自动选择。但注意 Flash 空间占用。

### Q: 字体太小看不清？

A: 建议使用 16px 或 18px，在 240x240 屏幕上效果较好。

### Q: 字体太大占用空间？

A: 可以：
- 减小字体大小（14px）
- 使用 2bpp 代替 4bpp
- 只包含需要的字符（symbols 方式）

---

## 最佳实践

**推荐配置**（平衡清晰度和空间）:
```
Size: 16px
Bpp:  2 bit-per-pixel
```

**小屏幕优化**（节省空间）:
```
Size: 14px
Bpp:  2 bit-per-pixel
```

**清晰优先**（需要更多空间）:
```
Size: 18px
Bpp:  4 bit-per-pixel
```
