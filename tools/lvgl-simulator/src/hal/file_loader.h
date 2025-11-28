/**
 * @file file_loader.h
 * RGB565二进制图片加载器
 */

#ifndef FILE_LOADER_H
#define FILE_LOADER_H

#include "lvgl/lvgl.h"
#include <stdbool.h>

/**
 * 加载LVGL格式的二进制图片文件
 * @param path 文件路径
 * @param img_dsc 图片描述符指针（输出）
 * @return 成功返回true
 */
bool file_loader_load_bin_image(const char *path, lv_img_dsc_t *img_dsc);

/**
 * 释放加载的图片资源
 * @param img_dsc 图片描述符指针
 */
void file_loader_free_image(lv_img_dsc_t *img_dsc);

#endif /* FILE_LOADER_H */
