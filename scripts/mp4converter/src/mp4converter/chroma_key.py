"""
透明底抠图模块

自动将绿底图片转换成透明底图片。
支持智能色键检测、边缘羽化和抗锯齿处理。
"""

from dataclasses import dataclass
from typing import Tuple, Optional, List
from PIL import Image, ImageFilter
import numpy as np
import cv2


@dataclass
class ChromaKeyConfig:
    """色键抠图配置"""
    target_color: Tuple[int, int, int] = (0, 255, 0)  # 目标颜色 (R, G, B)
    threshold: int = 50  # 颜色阈值
    softness: int = 2  # 边缘柔和度
    feather_radius: int = 1  # 羽化半径

    @property
    def enabled(self) -> bool:
        """是否启用抠图功能"""
        return True


class ChromaKeyError(Exception):
    """抠图处理错误"""
    pass


class ChromaKeyProcessor:
    """色键抠图处理器

    自动检测绿底并转换为透明背景。
    支持智能边缘处理和抗锯齿。
    """

    # 常见的绿幕颜色范围
    GREEN_RANGES = [
        (0, 100),    # 红色范围
        (150, 255),  # 绿色范围
        (0, 100)     # 蓝色范围
    ]

    def __init__(self, config: Optional[ChromaKeyConfig] = None):
        """初始化抠图处理器

        Args:
            config: 抠图配置，None则使用默认配置
        """
        self.config = config or ChromaKeyConfig()

    def analyze_image_dominant_color(self, image: Image.Image) -> Tuple[int, int, int]:
        """分析图像中的主要颜色

        Args:
            image: 输入图像

        Returns:
            Tuple[int, int, int]: 主要RGB颜色
        """
        try:
            # 转换为numpy数组
            img_array = np.array(image)

            # 重塑为像素列表
            pixels = img_array.reshape(-1, 3)

            # 计算每个颜色的出现频率
            unique_colors, counts = np.unique(pixels, axis=0, return_counts=True)

            # 找到出现频率最高的颜色
            dominant_idx = np.argmax(counts)
            dominant_color = tuple(unique_colors[dominant_idx])

            return dominant_color

        except Exception as e:
            raise ChromaKeyError(f"分析图像主要颜色失败: {e}")

    def is_green_screen_color(self, color: Tuple[int, int, int]) -> bool:
        """判断是否为绿幕颜色

        Args:
            color: RGB颜色

        Returns:
            bool: 是否为绿幕颜色
        """
        r, g, b = color

        # 检查是否符合绿色幕布的特征
        # 绿色分量明显大于红色和蓝色分量
        return (g > r * 1.5 and g > b * 1.5 and
                g > 100 and r < 100 and b < 100)

    def auto_detect_chroma_key(self, image: Image.Image) -> Tuple[int, int, int]:
        """自动检测色键颜色

        Args:
            image: 输入图像

        Returns:
            Tuple[int, int, int]: 检测到的色键颜色
        """
        try:
            # 分析图像主要颜色
            dominant_color = self.analyze_image_dominant_color(image)

            # 如果主要颜色符合绿色幕布特征，则使用它
            if self.is_green_screen_color(dominant_color):
                return dominant_color

            # 否则使用预设的绿色
            return self.config.target_color

        except Exception as e:
            print(f"自动检测色键失败，使用默认绿色: {e}")
            return self.config.target_color

    def create_alpha_mask(self, image: Image.Image,
                         target_color: Tuple[int, int, int]) -> Image.Image:
        """创建透明度遮罩

        改进的抠图算法，减少绿边问题：
        1. 使用HSV色彩空间进行更精确的绿色检测
        2. 多层阈值处理
        3. 边缘绿色像素清理
        4. 色彩溢出抑制

        Args:
            image: 输入图像
            target_color: 目标颜色

        Returns:
            Image.Image: 透明度遮罩
        """
        try:
            # 转换为numpy数组
            img_array = np.array(image)
            h, w = img_array.shape[:2]

            # 方法1: 传统RGB距离检测
            target_array = np.array(target_color)
            rgb_distance = np.sqrt(
                np.sum((img_array.astype(float) - target_array.astype(float)) ** 2, axis=2)
            )
            rgb_mask = (rgb_distance > self.config.threshold).astype(np.float32)

            # 方法2: HSV色彩空间检测（对绿色更敏感）
            img_hsv = cv2.cvtColor(img_array, cv2.COLOR_RGB2HSV)

            # 绿色的HSV范围
            # H: 35-85 (绿色色相范围)
            # S: 30-255 (饱和度)
            # V: 30-255 (明度)
            lower_green = np.array([35, 30, 30])
            upper_green = np.array([85, 255, 255])

            # 创建绿色掩码
            green_mask = cv2.inRange(img_hsv, lower_green, upper_green)
            green_mask = 1 - (green_mask / 255.0)  # 反转：非绿色为1

            # 方法3: 绿色通道专门处理（减少绿边，但保持边缘完整）
            r, g, b = img_array[:, :, 0], img_array[:, :, 1], img_array[:, :, 2]

            # 更温和的绿色主导度计算：只对非常明显的绿色进行抑制
            green_dominance = np.maximum(0,
                (g - np.maximum(r, b) * 1.5) / 255.0  # 增加阈值，减少影响范围
            )

            # 更严格的边缘检测：只处理明显的绿边像素
            edge_mask = self._detect_green_edges(img_array, target_color)

            # 组合所有遮罩
            combined_mask = rgb_mask * green_mask

            # 更温和的绿色抑制：只对强绿色像素进行抑制
            green_suppression = 1.0 - np.minimum(1.0, green_dominance * 1.0)  # 降低强度
            combined_mask *= green_suppression

            # 更保守的边缘清理：只清理明显的绿边
            combined_mask *= np.maximum(0.7, 1.0 - edge_mask)  # 保留至少70%的不透明度

            # 确保遮罩值在0-1范围内
            combined_mask = np.clip(combined_mask, 0, 1)

            # 转换为8位图像
            mask = (combined_mask * 255).astype(np.uint8)

            # 应用边缘柔和处理
            if self.config.softness > 0:
                # 使用高斯模糊软化边缘
                mask = Image.fromarray(mask)
                mask = mask.filter(
                    ImageFilter.GaussianBlur(radius=self.config.softness)
                )
                mask = np.array(mask)

            return Image.fromarray(mask, mode='L')

        except Exception as e:
            raise ChromaKeyError(f"创建透明度遮罩失败: {e}")

    def _detect_green_edges(self, img_array: np.ndarray, target_color: Tuple[int, int, int]) -> np.ndarray:
        """检测可能存在绿边的像素

        Args:
            img_array: RGB图像数组
            target_color: 目标绿色

        Returns:
            np.ndarray: 边缘遮罩 (0-1范围)
        """
        try:
            h, w = img_array.shape[:2]
            r, g, b = img_array[:, :, 0], img_array[:, :, 1], img_array[:, :, 2]

            # 计算与目标绿色的距离
            target_r, target_g, target_b = target_color
            color_diff = np.sqrt(
                (r.astype(float) - target_r) ** 2 +
                (g.astype(float) - target_g) ** 2 +
                (b.astype(float) - target_b) ** 2
            )

            # 绿色通道强度
            green_intensity = g.astype(float) / 255.0

            # 边缘条件：更严格的条件，只处理明显的绿边像素
            edge_condition = (
                (color_diff < 60) &  # 更严格的距离要求
                (green_intensity > 0.5) &  # 更高的绿色强度要求
                (g > r * 1.3) &  # 更强的绿色主导度
                (g > b * 1.3)  # 更强的绿色主导度
            )

            # 轻微膨胀边缘区域
            kernel = np.ones((3, 3), np.uint8)
            edge_mask = cv2.dilate(edge_condition.astype(np.uint8), kernel, iterations=1)

            # 转换为浮点数并应用柔和边缘
            edge_mask = edge_mask.astype(np.float32) * 0.7  # 减少强度

            return edge_mask

        except Exception:
            # 如果边缘检测失败，返回全零遮罩
            return np.zeros((img_array.shape[0], img_array.shape[1]), dtype=np.float32)

    def apply_feather_edges(self, image: Image.Image,
                           mask: Image.Image) -> Tuple[Image.Image, Image.Image]:
        """应用边缘羽化效果

        Args:
            image: 输入图像
            mask: 透明度遮罩

        Returns:
            Tuple[Image.Image, Image.Image]: 羽化后的图像和遮罩
        """
        try:
            if self.config.feather_radius <= 0:
                return image, mask

            # 对遮罩应用羽化
            feathered_mask = mask.filter(
                ImageFilter.GaussianBlur(radius=self.config.feather_radius)
            )

            return image, feathered_mask

        except Exception as e:
            print(f"边缘羽化处理失败，使用原始遮罩: {e}")
            return image, mask

    def remove_background(self, image: Image.Image,
                         auto_detect: bool = True) -> Image.Image:
        """去除背景，生成透明背景图像

        Args:
            image: 输入图像
            auto_detect: 是否自动检测色键颜色

        Returns:
            Image.Image: 透明背景图像
        """
        try:
            # 确保图像为RGB格式
            if image.mode != 'RGB':
                image = image.convert('RGB')

            # 确定目标颜色
            if auto_detect:
                target_color = self.auto_detect_chroma_key(image)
                print(f"自动检测到色键颜色: {target_color}")
            else:
                target_color = self.config.target_color

            # 创建透明度遮罩
            mask = self.create_alpha_mask(image, target_color)

            # 应用边缘羽化
            image, mask = self.apply_feather_edges(image, mask)

            # 创建透明背景图像
            if image.mode != 'RGBA':
                rgba_image = Image.new('RGBA', image.size, (0, 0, 0, 0))
                rgba_image.paste(image, mask=mask)
            else:
                rgba_image = image.copy()

            return rgba_image

        except Exception as e:
            if isinstance(e, ChromaKeyError):
                raise
            raise ChromaKeyError(f"去除背景失败: {e}")

    def process_images(self, images: List[Image.Image],
                      auto_detect: bool = True) -> List[Image.Image]:
        """批量处理图像

        Args:
            images: 图像列表
            auto_detect: 是否自动检测色键颜色

        Returns:
            List[Image.Image]: 处理后的图像列表
        """
        processed_images = []

        for i, image in enumerate(images):
            try:
                processed_image = self.remove_background(image, auto_detect)
                processed_images.append(processed_image)
            except Exception as e:
                print(f"处理第 {i+1} 张图像失败: {e}")
                # 使用原始图像
                if image.mode != 'RGBA':
                    rgba_image = Image.new('RGBA', image.size, (0, 0, 0, 0))
                    rgba_image.paste(image)
                    processed_images.append(rgba_image)
                else:
                    processed_images.append(image.copy())

        return processed_images

    def preview_chroma_key_detection(self, image: Image.Image) -> dict:
        """预览色键检测结果

        Args:
            image: 输入图像

        Returns:
            dict: 检测结果信息
        """
        try:
            # 分析图像主要颜色
            dominant_color = self.analyze_image_dominant_color(image)

            # 检测是否为绿幕
            is_green_screen = self.is_green_screen_color(dominant_color)

            # 自动检测色键颜色
            detected_color = self.auto_detect_chroma_key(image)

            # 计算绿色像素比例
            img_array = np.array(image)
            green_pixels = 0
            total_pixels = img_array.shape[0] * img_array.shape[1]

            for r, g, b in img_array.reshape(-1, 3):
                if self.is_green_screen_color((r, g, b)):
                    green_pixels += 1

            green_ratio = green_pixels / total_pixels

            return {
                'dominant_color': dominant_color,
                'is_green_screen': is_green_screen,
                'detected_chroma_key': detected_color,
                'green_pixel_ratio': green_ratio,
                'recommendation': '建议使用自动抠图' if green_ratio > 0.3 else '可能不是绿幕图像'
            }

        except Exception as e:
            raise ChromaKeyError(f"预览色键检测失败: {e}")

    @staticmethod
    def create_config(target_color: Optional[Tuple[int, int, int]] = None,
                     threshold: int = 50,
                     softness: int = 2,
                     feather_radius: int = 1) -> ChromaKeyConfig:
        """创建色键配置

        Args:
            target_color: 目标颜色 (R, G, B)
            threshold: 颜色阈值
            softness: 边缘柔和度
            feather_radius: 羽化半径

        Returns:
            ChromaKeyConfig: 色键配置
        """
        return ChromaKeyConfig(
            target_color=target_color or (0, 255, 0),
            threshold=threshold,
            softness=softness,
            feather_radius=feather_radius
        )

    @staticmethod
    def create_green_screen_config() -> ChromaKeyConfig:
        """创建绿幕抠图配置

        Returns:
            ChromaKeyConfig: 绿幕抠图配置
        """
        return ChromaKeyConfig(
            target_color=(0, 255, 0),
            threshold=60,
            softness=3,
            feather_radius=2
        )

    @staticmethod
    def create_blue_screen_config() -> ChromaKeyConfig:
        """创建蓝幕抠图配置

        Returns:
            ChromaKeyConfig: 蓝幕抠图配置
        """
        return ChromaKeyConfig(
            target_color=(0, 0, 255),
            threshold=60,
            softness=3,
            feather_radius=2
        )