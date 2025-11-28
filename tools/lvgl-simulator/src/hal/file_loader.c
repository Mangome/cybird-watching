/**
 * @file file_loader.c
 * RGB565二进制图片加载器实现
 */

#include "file_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// 静态缓冲区，用于存储图片数据
static uint8_t *image_data_buffer = NULL;
static size_t buffer_size = 0;

/**
 * 加载LVGL格式的二进制图片文件
 */
bool file_loader_load_bin_image(const char *path, lv_img_dsc_t *img_dsc)
{
    if (!path || !img_dsc) {
        fprintf(stderr, "Invalid parameters\n");
        return false;
    }
    
    // 打开文件
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open file: %s\n", path);
        return false;
    }
    
    // 读取文件大小
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (file_size < 12) {
        fprintf(stderr, "File too small (< 12 bytes): %s\n", path);
        fclose(fp);
        return false;
    }
    
    // 释放之前的缓冲区
    if (image_data_buffer) {
        free(image_data_buffer);
        image_data_buffer = NULL;
    }
    
    // 分配新缓冲区
    image_data_buffer = (uint8_t *)malloc(file_size);
    if (!image_data_buffer) {
        fprintf(stderr, "Failed to allocate memory: %ld bytes\n", file_size);
        fclose(fp);
        return false;
    }
    
    // 读取整个文件
    size_t bytes_read = fread(image_data_buffer, 1, file_size, fp);
    fclose(fp);
    
    if (bytes_read != file_size) {
        fprintf(stderr, "Failed to read file completely\n");
        free(image_data_buffer);
        image_data_buffer = NULL;
        return false;
    }
    
    // 解析LVGL图像头部（12字节）
    uint32_t header_cf;
    uint16_t width, height;
    uint32_t data_size;
    
    memcpy(&header_cf, image_data_buffer, 4);
    memcpy(&width, image_data_buffer + 4, 2);
    memcpy(&height, image_data_buffer + 6, 2);
    memcpy(&data_size, image_data_buffer + 8, 4);
    
    uint8_t cf = header_cf & 0xFF;
    uint8_t always_zero = (header_cf >> 8) & 0xFF;
    
    printf("Image header parsed:\n");
    printf("  cf = %u (should be 4 for RGB565)\n", cf);
    printf("  always_zero = %u\n", always_zero);
    printf("  width = %u\n", width);
    printf("  height = %u\n", height);
    printf("  data_size = %u\n", data_size);
    printf("  file_size = %ld\n", file_size);
    
    // 验证格式
    if (cf != 4) {
        fprintf(stderr, "Unsupported color format: %u (expected 4 for RGB565)\n", cf);
        free(image_data_buffer);
        image_data_buffer = NULL;
        return false;
    }
    
    // 验证数据大小
    uint32_t expected_data_size = width * height * 2;
    if (data_size != expected_data_size) {
        fprintf(stderr, "Warning: data_size mismatch (header=%u, expected=%u)\n",
                data_size, expected_data_size);
    }
    
    if (file_size < 12 + data_size) {
        fprintf(stderr, "File size mismatch (file=%ld, expected=%u)\n",
                file_size, 12 + data_size);
        free(image_data_buffer);
        image_data_buffer = NULL;
        return false;
    }
    
    // 填充LVGL图像描述符
    memset(img_dsc, 0, sizeof(lv_img_dsc_t));
    img_dsc->header.always_zero = 0;
    img_dsc->header.w = width;
    img_dsc->header.h = height;
    img_dsc->header.cf = LV_IMG_CF_TRUE_COLOR; // RGB565
    img_dsc->data_size = data_size;
    img_dsc->data = image_data_buffer + 12; // 跳过12字节头部
    
    buffer_size = file_size;
    
    printf("Image loaded successfully: %ux%u, %u bytes\n", width, height, data_size);
    return true;
}

/**
 * 释放加载的图片资源
 */
void file_loader_free_image(lv_img_dsc_t *img_dsc)
{
    if (image_data_buffer) {
        free(image_data_buffer);
        image_data_buffer = NULL;
        buffer_size = 0;
    }
    
    if (img_dsc) {
        memset(img_dsc, 0, sizeof(lv_img_dsc_t));
    }
}
