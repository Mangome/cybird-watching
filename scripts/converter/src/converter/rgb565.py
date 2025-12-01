"""
RGB565图片转换器
将PNG、JPG、JPEG等格式图片转换为RGB565格式
"""

from PIL import Image
import numpy as np
from pathlib import Path
from typing import Tuple, Optional
import struct


class RGB565Converter:
    """RGB565格式转换器"""

    @staticmethod
    def rgb_to_rgb565(r: int, g: int, b: int) -> int:
        """
        将RGB值转换为标准RGB565格式

        Args:
            r: 红色分量 (0-255)
            g: 绿色分量 (0-255)
            b: 蓝色分量 (0-255)

        Returns:
            标准RGB565格式的16位颜色值
        """
        # 将8位颜色转换为5位或6位
        r5 = (r >> 3) & 0x1F    # 红色5位
        g6 = (g >> 2) & 0x3F    # 绿色6位
        b5 = (b >> 3) & 0x1F    # 蓝色5位

        # 组合为标准RGB565格式 (RRRRRGGGGGGBBBBB)
        return (r5 << 11) | (g6 << 5) | b5

    @staticmethod
    def convert_image_to_rgb565(image_path: Path, output_path: Path,
                               max_size: Optional[Tuple[int, int]] = None) -> bool:
        """
        将图片转换为RGB565格式并保存为LVGL 7.9.1兼容的二进制文件

        Args:
            image_path: 输入图片路径
            output_path: 输出文件路径
            max_size: 最大尺寸 (width, height)，如果图片超过此尺寸则会等比缩放

        Returns:
            转换是否成功
        """
        try:
            # 打开图片
            with Image.open(image_path) as img:
                # 转换为RGB模式
                if img.mode != 'RGB':
                    img = img.convert('RGB')

                # 调整尺寸如果指定了最大尺寸
                if max_size:
                    img.thumbnail(max_size, Image.Resampling.LANCZOS)

                # 获取图片尺寸
                width, height = img.size

                # 转换为numpy数组
                pixels = np.array(img)

                # 创建RGB565数据数组
                rgb565_data = []

                for y in range(height):
                    for x in range(width):
                        r, g, b = pixels[y, x]
                        
                        # 重要：确保转换为Python int，避免numpy.uint8溢出问题
                        r = int(r)
                        g = int(g)
                        b = int(b)
                        
                        rgb565 = RGB565Converter.rgb_to_rgb565(r, g, b)
                        rgb565_data.append(rgb565)

                # 写入LVGL 9.x兼容的二进制文件
                with open(output_path, 'wb') as f:
                    # LVGL 9.x图像头部
                    # cf (color format): LV_COLOR_FORMAT_RGB565 = 0x12 (18) for RGB565
                    LV_COLOR_FORMAT_RGB565 = 0x12
                    magic = 0x37  # LVGL 9.x magic number '7' (LVGL_VERSION_MAJOR = 9 -> '7')
                    header_cf = (magic << 24) | (LV_COLOR_FORMAT_RGB565)  # 32-bit header

                    # 写入LVGL 9.x图像头部 - 参考lv_bin_decoder.c期望的格式
                    f.write(struct.pack('<I', header_cf))  # 4字节: magic + cf
                    f.write(struct.pack('<I', 0))  # 4字节: flags (32-bit)
                    f.write(struct.pack('<HH', width, height))  # 4字节: width + height
                    f.write(struct.pack('<I', 0))  # 4字节: stride (calculated automatically)
                    f.write(struct.pack('<I', 0))  # 4字节: reserved_2
                    data_size = len(rgb565_data) * 2  # RGB565每像素2字节
                    f.write(struct.pack('<I', data_size))  # 4字节: data_size
                    
                    # 写入RGB565像素数据
                    for pixel in rgb565_data:
                        f.write(struct.pack('<H', pixel))

                return True

        except Exception as e:
            print(f"转换图片 {image_path} 时出错: {e}")
            return False

    @staticmethod
    def convert_image_to_c_array(image_path: Path, output_path: Path,
                                array_name: Optional[str] = None,
                                max_size: Optional[Tuple[int, int]] = None) -> bool:
        """
        将图片转换为RGB565格式的C数组

        Args:
            image_path: 输入图片路径
            output_path: 输出C文件路径
            array_name: 数组名称，如果为None则使用文件名
            max_size: 最大尺寸 (width, height)，如果图片超过此尺寸则会等比缩放

        Returns:
            转换是否成功
        """
        try:
            # 打开图片
            with Image.open(image_path) as img:
                # 转换为RGB模式
                if img.mode != 'RGB':
                    img = img.convert('RGB')

                # 调整尺寸如果指定了最大尺寸
                if max_size:
                    img.thumbnail(max_size, Image.Resampling.LANCZOS)

                # 获取图片尺寸
                width, height = img.size

                # 转换为numpy数组
                pixels = np.array(img)

                # 生成数组名称
                if array_name is None:
                    array_name = image_path.stem.replace('-', '_').replace(' ', '_')

                # 生成C数组内容
                c_content = []
                c_content.append(f"// RGB565 Image Data")
                c_content.append(f"// Generated from: {image_path.name}")
                c_content.append(f"// Size: {width}x{height}")
                c_content.append(f"")
                c_content.append(f"#include <stdint.h>")
                c_content.append(f"")
                c_content.append(f"// Image dimensions")
                c_content.append(f"const uint16_t {array_name}_width = {width};")
                c_content.append(f"const uint16_t {array_name}_height = {height};")
                c_content.append(f"")
                c_content.append(f"// RGB565 pixel data")
                c_content.append(f"const uint16_t {array_name}_data[] = {{")

                # 转换像素数据
                pixel_data = []
                for y in range(height):
                    row_data = []
                    for x in range(width):
                        r, g, b = pixels[y, x]
                        rgb565 = RGB565Converter.rgb_to_rgb565(r, g, b)
                        row_data.append(f"0x{rgb565:04X}")
                    pixel_data.append("    " + ", ".join(row_data))

                c_content.append(",\n".join(pixel_data))
                c_content.append("};")

                # 写入C文件
                with open(output_path, 'w', encoding='utf-8') as f:
                    f.write("\n".join(c_content))

                return True

        except Exception as e:
            print(f"转换图片 {image_path} 为C数组时出错: {e}")
            return False