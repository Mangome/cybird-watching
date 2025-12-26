#include "font_manager_c.h"
#include "font_manager.h"
#include <Arduino.h>

extern "C" {

lv_font_t* font_manager_load(const char* font_name) {
    Serial.printf("[FONT_C] Calling font_manager_load('%s')\n", font_name ? font_name : "NULL");
    return FontManager::getInstance().loadFont(font_name);
}

void font_manager_destroy(const char* font_name) {
    FontManager::getInstance().destroyFont(font_name);
}

int font_manager_is_loaded(const char* font_name) {
    return FontManager::getInstance().isFontLoaded(font_name) ? 1 : 0;
}

} // extern "C"
