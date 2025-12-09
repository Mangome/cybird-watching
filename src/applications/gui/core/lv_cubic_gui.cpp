/*********************
 *      INCLUDES
 *********************/
#include "lv_cubic_gui.h"
#include "Arduino.h"
#include "SD.h"
#include "system/logging/log_manager.h"
#include "config/version.h"

// C语言全局变量
extern "C" {
	lv_obj_t* scr;
}

// C++内部使用的静态变量
static lv_obj_t* logo_img = NULL;
static lv_obj_t* logo_scr = NULL;
static lv_obj_t* logo_version_label = NULL;  // 版本号标签
static lv_image_dsc_t* logo_img_dsc = NULL;
static uint8_t* logo_img_data = NULL;
static uint32_t logo_show_time = 0;  // logo显示的开始时间
static bool logo_visible = false;    // logo是否可见
static bool logo_timeout_enabled = false;  // logo超时检查是否启用（默认禁用，由外部启用）

// 前向声明
static void hideLogo();

// 检查并隐藏logo（只在超时检查启用后才生效）
static void checkAndHideLogo()
{
	if (!logo_visible || !logo_timeout_enabled) {
		return;
	}
	
	uint32_t current_time = millis();
	if (current_time - logo_show_time >= 10000) {  // 10秒超时保护
		LOG_INFO("GUI", "Logo display timeout (10s), switching to bird scene...");
		hideLogo();
	}
}

// 隐藏logo并切换到小鸟界面（内部函数）
static void hideLogo()
{
	if (!logo_visible) {
		return;
	}
	
	LOG_INFO("GUI", "Hiding logo and switching to bird scene...");
	
	// 切换到小鸟展示界面
	extern lv_ui guider_ui;
	if (guider_ui.scenes != NULL) {
		lv_scr_load(guider_ui.scenes);
		LOG_INFO("GUI", "Switched to bird scene");
	}
	
	// 删除logo屏幕（在切换后删除以避免闪烁）
	if (logo_scr != NULL) {
		lv_obj_del(logo_scr);
		logo_scr = NULL;
		logo_img = NULL;  // logo_img 是 logo_scr 的子对象,会被自动删除
		logo_version_label = NULL;  // logo_version_label 也是 logo_scr 的子对象,会被自动删除
		LOG_INFO("GUI", "Logo screen deleted");
	}
	
	// 释放logo图片内存
	if (logo_img_data != NULL) {
		free(logo_img_data);
		logo_img_data = NULL;
	}
	if (logo_img_dsc != NULL) {
		free(logo_img_dsc);
		logo_img_dsc = NULL;
		LOG_INFO("GUI", "Logo memory freed");
	}
	
	logo_visible = false;
}

// 从SD卡手动加载logo图片
static bool load_logo_from_sd(const char* file_path)
{
	File file = SD.open(file_path);
	if (!file) {
		LOG_ERROR("GUI", "Failed to open logo file: " + String(file_path));
		return false;
	}

	size_t file_size = file.size();
	
	if (file_size < 32) { // LVGL 9.x最小头部大小
		LOG_ERROR("GUI", "Logo file too small");
		file.close();
		return false;
	}

	// 读取LVGL 9.x头部
	uint32_t header_cf, flags, stride, reserved_2, data_size;
	uint16_t width, height;

	if (file.read((uint8_t*)&header_cf, sizeof(header_cf)) != sizeof(header_cf) ||
		file.read((uint8_t*)&flags, sizeof(flags)) != sizeof(flags) ||
		file.read((uint8_t*)&width, sizeof(width)) != sizeof(width) ||
		file.read((uint8_t*)&height, sizeof(height)) != sizeof(height) ||
		file.read((uint8_t*)&stride, sizeof(stride)) != sizeof(stride) ||
		file.read((uint8_t*)&reserved_2, sizeof(reserved_2)) != sizeof(reserved_2) ||
		file.read((uint8_t*)&data_size, sizeof(data_size)) != sizeof(data_size)) {
		LOG_ERROR("GUI", "Failed to read logo header");
		file.close();
		return false;
	}

	// 验证LVGL 9.x文件格式
	uint8_t color_format = header_cf & 0xFF;
	uint8_t magic = (header_cf >> 24) & 0xFF;

	if (color_format != 0x12) { // LVGL 9.x: LV_COLOR_FORMAT_RGB565 = 0x12
		LOG_ERROR("GUI", "Invalid color format: 0x" + String(color_format, HEX));
		file.close();
		return false;
	}

	if (magic != 0x37) { // LVGL 9.x magic number
		LOG_ERROR("GUI", "Invalid magic number: 0x" + String(magic, HEX));
		file.close();
		return false;
	}

	// 分配内存
	logo_img_dsc = (lv_image_dsc_t*)malloc(sizeof(lv_image_dsc_t));
	if (!logo_img_dsc) {
		LOG_ERROR("GUI", "Failed to allocate logo descriptor");
		file.close();
		return false;
	}

	logo_img_data = (uint8_t*)malloc(data_size);
	if (!logo_img_data) {
		LOG_ERROR("GUI", "Failed to allocate logo data");
		free(logo_img_dsc);
		logo_img_dsc = NULL;
		file.close();
		return false;
	}

	// 读取像素数据
	size_t bytes_read = file.read(logo_img_data, data_size);
	file.close();

	if (bytes_read != data_size) {
		LOG_ERROR("GUI", "Failed to read logo data: " + String(bytes_read) + "/" + String(data_size));
		free(logo_img_dsc);
		free(logo_img_data);
		logo_img_dsc = NULL;
		logo_img_data = NULL;
		return false;
	}

	// 设置LVGL图像描述符 - LVGL 9.x格式
	logo_img_dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
	logo_img_dsc->header.cf = color_format;
	logo_img_dsc->header.flags = flags;
	logo_img_dsc->header.w = width;
	logo_img_dsc->header.h = height;
	logo_img_dsc->header.stride = stride;
	logo_img_dsc->header.reserved_2 = reserved_2;
	logo_img_dsc->data_size = data_size;
	logo_img_dsc->data = logo_img_data;

	LOG_INFO("GUI", "Logo loaded: " + String(width) + "x" + String(height));
	return true;
}

// 导出的C接口函数
extern "C" {

void lv_init_gui(void)
{
	// 先尝试从SD卡加载logo图片（保持黑屏状态）
	if (load_logo_from_sd("/static/logo.bin")) {
		LOG_INFO("GUI", "Logo loaded successfully, displaying...");
		
		// 创建logo专用屏幕
		logo_scr = lv_obj_create(NULL);
		
		static lv_style_t default_style;
		lv_style_init(&default_style);
		lv_style_set_bg_color(&default_style, lv_color_black());
		lv_style_set_bg_opa(&default_style, LV_OPA_COVER);
		lv_obj_add_style(logo_scr, &default_style, 0);
		
		// 创建图片对象
		logo_img = lv_image_create(logo_scr);
		
		// 设置图片源
		lv_image_set_src(logo_img, logo_img_dsc);
		
		// 计算缩放比例 - 缩放到240x240
		// LVGL缩放：256 = 1.0x, 512 = 2.0x
		// 假设原图尺寸为120x120，需要2倍缩放到240x240
		uint16_t width = logo_img_dsc->header.w;
		uint16_t height = logo_img_dsc->header.h;
		uint16_t zoom_factor = (240 * 256) / width; // 计算缩放比例
		
		// 设置缩放中心点为图像中心
		lv_img_set_pivot(logo_img, width / 2, height / 2);
		
		// 应用缩放
		lv_img_set_zoom(logo_img, zoom_factor);
		
		// 确保对象可见
		lv_obj_clear_flag(logo_img, LV_OBJ_FLAG_HIDDEN);
		
		// 居中显示
		lv_obj_center(logo_img);
		
		// 创建版本号标签
		logo_version_label = lv_label_create(logo_scr);
		lv_label_set_text(logo_version_label, FIRMWARE_VERSION_FULL);
		
		// 设置版本号样式
		lv_obj_set_style_text_color(logo_version_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
		lv_obj_set_style_text_font(logo_version_label, &lv_font_notosanssc_18, LV_PART_MAIN);
		
		// 位置：右下角，距离边缘 10 像素
		lv_obj_align(logo_version_label, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
		
		// 确保版本号可见
		lv_obj_clear_flag(logo_version_label, LV_OBJ_FLAG_HIDDEN);
		
		// 加载logo屏幕（从黑屏切换到logo）
		lv_scr_load(logo_scr);
		
		// 记录显示时间
		logo_show_time = millis();
		logo_visible = true;
		logo_timeout_enabled = false;  // 初始禁用超时检查，等待资源扫描完成
		
		LOG_INFO("GUI", "Logo screen loaded with version " + String(FIRMWARE_VERSION_FULL) + " (timeout check disabled during resource scan)");
	} else {
		LOG_WARN("GUI", "Failed to load logo, showing bird scene directly");
		// 如果logo加载失败,直接加载小鸟界面(不创建logo屏幕)
		extern lv_ui guider_ui;
		if (guider_ui.scenes != NULL) {
			lv_scr_load(guider_ui.scenes);
			LOG_INFO("GUI", "Bird scene loaded directly");
		}
	}
}

void lv_check_logo_timeout(void)
{
	// 检查并隐藏logo（如果超时）
	checkAndHideLogo();
}

void lv_hide_logo(void)
{
	// 立即隐藏logo（用于资源扫描完成后）
	// 如果logo还在显示，启用超时检查作为保护机制
	if (logo_visible) {
		logo_timeout_enabled = true;  // 启用超时检查
		LOG_INFO("GUI", "Resource scan completed, hiding logo and enabling timeout protection");
	}
	hideLogo();
}

} // extern "C"