#include "font_manager.h"
#include "system/logging/log_manager.h"

// 日志标签
#define LOG_TAG "FONT"

// 静态成员初始化
FontManager::FontManager() {
    LOG_INFO(LOG_TAG, "FontManager initialized");
}

FontManager::~FontManager() {
    destroyAllFonts();
}

FontManager& FontManager::getInstance() {
    static FontManager instance;
    return instance;
}

lv_font_t* FontManager::loadFont(const char* font_name) {
    if (!font_name || strlen(font_name) == 0) {
        LOG_ERROR(LOG_TAG, "Invalid font name: null or empty");
        return nullptr;
    }

    // 检查是否已加载
    auto it = loaded_fonts_.find(font_name);
    if (it != loaded_fonts_.end()) {
        return it->second;
    }

    // 构建字体文件路径：S:/fonts/notosanssc_16.fnt
    char font_path[128];
    snprintf(font_path, sizeof(font_path), "%s%s%s", FONT_DIR, font_name, FONT_EXT);

    LOG_INFO(LOG_TAG, String("Loading font: ") + font_name);

    // 使用 LVGL binfont API 加载字体
    lv_font_t* font = lv_binfont_create(font_path);
    
    if (!font) {
        LOG_ERROR(LOG_TAG, String("Failed to load font: ") + font_path);
        return nullptr;
    }

    // 注册到字体表
    loaded_fonts_[font_name] = font;
    
    LOG_INFO(LOG_TAG, String("Font loaded: ") + font_name);

    return font;
}

void FontManager::destroyFont(lv_font_t* font) {
    if (!font) {
        LOG_WARN(LOG_TAG, "Attempted to destroy null font pointer");
        return;
    }

    // 从注册表中查找并移除
    for (auto it = loaded_fonts_.begin(); it != loaded_fonts_.end(); ++it) {
        if (it->second == font) {
            String font_name = it->first.c_str();
            loaded_fonts_.erase(it);
            
            // 使用 LVGL API 销毁字体
            lv_binfont_destroy(font);
            
            LOG_INFO(LOG_TAG, String("Font destroyed: ") + font_name);
            return;
        }
    }

    LOG_WARN(LOG_TAG, "Font pointer not found in registry, destroying anyway");
    lv_binfont_destroy(font);
}

void FontManager::destroyFont(const char* font_name) {
    if (!font_name) {
        LOG_WARN(LOG_TAG, "Attempted to destroy font with null name");
        return;
    }

    auto it = loaded_fonts_.find(font_name);
    if (it == loaded_fonts_.end()) {
        LOG_WARN(LOG_TAG, String("Font not found: ") + font_name);
        return;
    }

    lv_font_t* font = it->second;
    loaded_fonts_.erase(it);
    
    // 使用 LVGL API 销毁字体
    lv_binfont_destroy(font);
    
    LOG_INFO(LOG_TAG, String("Font destroyed: ") + font_name);
}

void FontManager::destroyAllFonts() {
    if (loaded_fonts_.empty()) {
        return;
    }

    LOG_INFO(LOG_TAG, String("Destroying all fonts (") + String(loaded_fonts_.size()) + ")");

    for (auto& pair : loaded_fonts_) {
        lv_binfont_destroy(pair.second);
    }

    loaded_fonts_.clear();
    LOG_INFO(LOG_TAG, "All fonts destroyed");
}

bool FontManager::isFontLoaded(const char* font_name) const {
    if (!font_name) {
        return false;
    }
    return loaded_fonts_.find(font_name) != loaded_fonts_.end();
}

lv_font_t* FontManager::getFont(const char* font_name) const {
    if (!font_name) {
        return nullptr;
    }

    auto it = loaded_fonts_.find(font_name);
    if (it != loaded_fonts_.end()) {
        return it->second;
    }

    return nullptr;
}
