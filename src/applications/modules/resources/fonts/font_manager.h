#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

#include <Arduino.h>
#include <lvgl.h>
#include <map>
#include <string>

/**
 * FontManager - 字体动态加载管理器
 * 
 * 使用 LVGL 原生 binfont API 从 SD 卡按需加载字体文件
 * 字体文件存放在 SD 卡 /fonts/ 目录，通过 LVGL 驱动器 'S:' 访问
 * 
 * 特点：
 * - 节省固件 Flash 空间（~281KB）
 * - LVGL 自动管理字形缓存，内存占用小（~20-50KB）
 * - 支持运行时动态切换字体
 * - 完善的错误处理和日志记录
 */
class FontManager {
public:
    /**
     * 获取 FontManager 单例
     */
    static FontManager& getInstance();

    /**
     * 从 SD 卡加载字体
     * 
     * @param font_name 字体名称（不含扩展名），例如 "notosanssc_16"
     * @return 成功返回字体指针，失败返回 nullptr
     * 
     * 示例：
     *   lv_font_t* font = FontManager::getInstance().loadFont("notosanssc_16");
     *   if (font) {
     *       lv_obj_set_style_text_font(label, font, 0);
     *   }
     */
    lv_font_t* loadFont(const char* font_name);

    /**
     * 销毁字体并释放资源
     * 
     * @param font 要销毁的字体指针
     * 
     * 注意：不要销毁正在使用的字体，确保没有 UI 对象引用该字体
     */
    void destroyFont(lv_font_t* font);

    /**
     * 根据名称销毁字体
     * 
     * @param font_name 字体名称
     */
    void destroyFont(const char* font_name);

    /**
     * 销毁所有已加载的字体
     * 
     * 通常在系统关闭或重新初始化时调用
     */
    void destroyAllFonts();

    /**
     * 检查字体是否已加载
     * 
     * @param font_name 字体名称
     * @return 已加载返回 true
     */
    bool isFontLoaded(const char* font_name) const;

    /**
     * 获取已加载的字体
     * 
     * @param font_name 字体名称
     * @return 字体指针，未找到返回 nullptr
     */
    lv_font_t* getFont(const char* font_name) const;

    /**
     * 获取已加载字体数量
     */
    size_t getLoadedFontCount() const { return loaded_fonts_.size(); }

private:
    FontManager();
    ~FontManager();
    
    // 禁止拷贝和赋值
    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;

    // 已加载的字体注册表：字体名称 -> 字体指针
    std::map<std::string, lv_font_t*> loaded_fonts_;

    // SD 卡字体目录路径（LVGL 文件系统路径）
    // 注意：LVGL 9.x 的 FatFs 驱动需要带驱动器字母的路径
    static constexpr const char* FONT_DIR = "S:/fonts/";
    
    // 字体文件扩展名
    static constexpr const char* FONT_EXT = ".fnt";
};

// 便捷宏定义
#define FONT_MANAGER FontManager::getInstance()

#endif // FONT_MANAGER_H
