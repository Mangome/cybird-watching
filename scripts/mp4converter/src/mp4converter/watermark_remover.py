"""
水印处理模块

基于用户选择的基础区域填充方式，实现简单快速的水印去除功能。
支持手动指定区域、周围像素平均值填充和基础边缘平滑处理。
"""

from dataclasses import dataclass
from typing import Tuple, Optional, List
from PIL import Image, ImageFilter
import numpy as np


@dataclass
class Rectangle:
    """矩形区域数据类"""
    x: int
    y: int
    width: int
    height: int

    @property
    def left(self) -> int:
        """左边界"""
        return self.x

    @property
    def right(self) -> int:
        """右边界"""
        return self.x + self.width

    @property
    def top(self) -> int:
        """上边界"""
        return self.y

    @property
    def bottom(self) -> int:
        """下边界"""
        return self.y + self.height

    @property
    def center(self) -> Tuple[int, int]:
        """中心点坐标"""
        return (self.x + self.width // 2, self.y + self.height // 2)

    def contains(self, x: int, y: int) -> bool:
        """检查点是否在矩形内"""
        return self.left <= x < self.right and self.top <= y < self.bottom

    def get_border_points(self, image_size: Tuple[int, int],
                         border_width: int = 5) -> List[Tuple[int, int]]:
        """获取矩形周围的边框点

        Args:
            image_size: 图像尺寸 (width, height)
            border_width: 边框宽度

        Returns:
            List[Tuple[int, int]]: 边框点坐标列表
        """
        img_width, img_height = image_size
        points = []

        # 计算扩展区域
        left = max(0, self.left - border_width)
        right = min(img_width, self.right + border_width)
        top = max(0, self.top - border_width)
        bottom = min(img_height, self.bottom + border_width)

        # 收集边框点（不包括水印区域本身）
        for y in range(top, bottom):
            for x in range(left, right):
                if not self.contains(x, y):
                    points.append((x, y))

        return points

    def iter_points(self) -> List[Tuple[int, int]]:
        """遍历矩形内的所有点"""
        points = []
        for y in range(self.top, self.bottom):
            for x in range(self.left, self.right):
                points.append((x, y))
        return points


class WatermarkRemoverError(Exception):
    """水印去除错误"""
    pass


class WatermarkRemover:
    """水印去除器

    基于用户选择的基础区域填充方式实现水印去除。
    使用周围像素平均值进行填充，并应用基础平滑处理。
    """

    def __init__(self, border_width: int = 10,
                 smooth_kernel_size: int = 3,
                 blend_alpha: float = 0.3):
        """初始化水印去除器

        Args:
            border_width: 用于计算平均颜色的边框宽度
            smooth_kernel_size: 高斯模糊核大小（奇数）
            blend_alpha: 边缘融合的透明度 (0-1)
        """
        self.border_width = border_width
        self.smooth_kernel_size = smooth_kernel_size
        self.blend_alpha = blend_alpha

        # 确保核大小为奇数
        if smooth_kernel_size % 2 == 0:
            self.smooth_kernel_size = smooth_kernel_size + 1

    def parse_region_string(self, region_str: str, image_size: Tuple[int, int]) -> Rectangle:
        """解析区域字符串

        支持格式: "X,Y,WIDTH,HEIGHT"
        例如: "10,10,50,50" 表示左上角坐标(10,10)，尺寸50x50

        Args:
            region_str: 区域字符串
            image_size: 图像尺寸 (width, height)

        Returns:
            Rectangle: 矩形区域对象
        """
        try:
            parts = region_str.split(',')
            if len(parts) != 4:
                raise ValueError("区域格式应为: X,Y,WIDTH,HEIGHT")

            x = int(parts[0].strip())
            y = int(parts[1].strip())
            width = int(parts[2].strip())
            height = int(parts[3].strip())

            # 验证参数
            if width <= 0 or height <= 0:
                raise ValueError("宽度和高度必须大于0")

            if x < 0 or y < 0:
                raise ValueError("坐标不能为负数")

            img_width, img_height = image_size
            if x + width > img_width or y + height > img_height:
                raise ValueError("区域超出图像边界")

            return Rectangle(x=x, y=y, width=width, height=height)

        except ValueError as e:
            raise WatermarkRemoverError(f"解析区域字符串失败 '{region_str}': {e}")

    def calculate_average_color(self, image: Image.Image,
                              region: Rectangle) -> Tuple[int, int, int]:
        """计算区域周围像素的平均颜色

        Args:
            image: 输入图像
            region: 水印区域

        Returns:
            Tuple[int, int, int]: 平均RGB颜色
        """
        try:
            # 获取周围边框点
            border_points = region.get_border_points(image.size, self.border_width)

            if not border_points:
                # 如果没有周围点，使用整个图像的平均颜色
                img_array = np.array(image)
                avg_color = np.mean(img_array, axis=(0, 1))
                return tuple(map(int, avg_color))

            # 计算周围像素的平均颜色
            total_r, total_g, total_b = 0, 0, 0
            count = 0

            for x, y in border_points:
                r, g, b = image.getpixel((x, y))
                total_r += r
                total_g += g
                total_b += b
                count += 1

            avg_r = int(total_r / count)
            avg_g = int(total_g / count)
            avg_b = int(total_b / count)

            return (avg_r, avg_g, avg_b)

        except Exception as e:
            raise WatermarkRemoverError(f"计算平均颜色失败: {e}")

    def fill_region_with_color(self, image: Image.Image,
                             region: Rectangle,
                             color: Tuple[int, int, int]) -> Image.Image:
        """用指定颜色填充区域

        Args:
            image: 输入图像
            region: 要填充的区域
            color: 填充颜色 (R, G, B)

        Returns:
            Image.Image: 填充后的图像
        """
        try:
            result = image.copy()

            # 创建填充掩码
            mask = Image.new('L', image.size, 0)
            for x, y in region.iter_points():
                mask.putpixel((x, y), 255)

            # 应用填充
            result.paste(color, mask=mask)

            return result

        except Exception as e:
            raise WatermarkRemoverError(f"填充区域失败: {e}")

    def apply_smooth_blending(self, original_image: Image.Image,
                            filled_image: Image.Image,
                            region: Rectangle) -> Image.Image:
        """应用平滑边缘融合

        Args:
            original_image: 原始图像
            filled_image: 填充后的图像
            region: 处理区域

        Returns:
            Image.Image: 融合后的图像
        """
        try:
            result = filled_image.copy()

            # 创建扩展区域用于融合
            expanded_region = Rectangle(
                x=max(0, region.left - self.border_width),
                y=max(0, region.top - self.border_width),
                width=region.width + 2 * self.border_width,
                height=region.height + 2 * self.border_width
            )

            # 确保扩展区域不超出图像边界
            img_width, img_height = original_image.size
            if (expanded_region.right > img_width or
                expanded_region.bottom > img_height):
                expanded_region = Rectangle(
                    x=region.left,
                    y=region.top,
                    width=min(region.width, img_width - region.left),
                    height=min(region.height, img_height - region.top)
                )

            # 裁剪区域进行高斯模糊
            crop_box = (expanded_region.left, expanded_region.top,
                       expanded_region.right, expanded_region.bottom)

            original_crop = original_image.crop(crop_box)
            filled_crop = filled_image.crop(crop_box)

            # 应用高斯模糊
            blurred_filled = filled_crop.filter(
                ImageFilter.GaussianBlur(radius=self.smooth_kernel_size / 3)
            )

            # 创建渐变掩码
            mask = Image.new('L', blurred_filled.size, 0)

            # 在水印区域中心使用255（完全使用模糊图像）
            for y in range(expanded_region.height):
                for x in range(expanded_region.width):
                    # 检查是否在原始水印区域内
                    global_x = expanded_region.left + x
                    global_y = expanded_region.top + y

                    if region.contains(global_x, global_y):
                        # 计算到区域边缘的距离
                        dist_to_left = global_x - region.left
                        dist_to_right = region.right - global_x - 1
                        dist_to_top = global_y - region.top
                        dist_to_bottom = region.bottom - global_y - 1

                        # 最小距离到边缘
                        min_dist = min(dist_to_left, dist_to_right,
                                     dist_to_top, dist_to_bottom)

                        if min_dist >= 3:
                            mask.putpixel((x, y), 255)
                        else:
                            # 在边缘使用渐变
                            alpha = min(255, int((min_dist + 1) * 85))
                            mask.putpixel((x, y), alpha)

            # 应用掩码融合
            blended_crop = Image.blend(original_crop, blurred_filled,
                                    self.blend_alpha)
            blended_crop.paste(blurred_filled, mask=mask)

            # 将处理后的区域粘贴回结果图像
            result.paste(blended_crop, crop_box)

            return result

        except Exception as e:
            raise WatermarkRemoverError(f"应用平滑融合失败: {e}")

    def remove_watermark(self, image: Image.Image, region: Rectangle) -> Image.Image:
        """去除指定区域的水印

        基础水印去除算法：
        1. 分析指定区域周围的像素
        2. 计算周围像素的平均颜色
        3. 用平均颜色填充水印区域
        4. 应用基础平滑处理

        Args:
            image: 输入图像
            region: 水印区域

        Returns:
            Image.Image: 去除水印后的图像
        """
        try:
            # 确保图像为RGB格式
            if image.mode != 'RGB':
                image = image.convert('RGB')

            # 1. 计算周围像素的平均颜色
            avg_color = self.calculate_average_color(image, region)

            # 2. 用平均颜色填充水印区域
            filled_image = self.fill_region_with_color(image, region, avg_color)

            # 3. 应用平滑边缘融合
            result_image = self.apply_smooth_blending(image, filled_image, region)

            return result_image

        except Exception as e:
            if isinstance(e, WatermarkRemoverError):
                raise
            raise WatermarkRemoverError(f"去除水印失败: {e}")

    def remove_watermark_from_region_string(self, image: Image.Image,
                                          region_str: str) -> Image.Image:
        """从区域字符串去除水印

        Args:
            image: 输入图像
            region_str: 区域字符串 (格式: "X,Y,WIDTH,HEIGHT")

        Returns:
            Image.Image: 去除水印后的图像
        """
        try:
            # 解析区域字符串
            region = self.parse_region_string(region_str, image.size)

            # 去除水印
            return self.remove_watermark(image, region)

        except Exception as e:
            if isinstance(e, WatermarkRemoverError):
                raise
            raise WatermarkRemoverError(f"从区域字符串去除水印失败: {e}")

    @staticmethod
    def create_default_remover() -> 'WatermarkRemover':
        """创建默认的水印去除器

        Returns:
            WatermarkRemover: 默认配置的水印去除器
        """
        return WatermarkRemover(
            border_width=10,
            smooth_kernel_size=3,
            blend_alpha=0.3
        )

    @staticmethod
    def validate_region(region: Rectangle, image_size: Tuple[int, int]) -> bool:
        """验证区域是否有效

        Args:
            region: 要验证的区域
            image_size: 图像尺寸

        Returns:
            bool: 区域是否有效
        """
        img_width, img_height = image_size

        # 检查基本参数
        if region.width <= 0 or region.height <= 0:
            return False

        if region.x < 0 or region.y < 0:
            return False

        # 检查边界
        if region.right > img_width or region.bottom > img_height:
            return False

        return True