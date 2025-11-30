"""
图像处理和像素级操作模块

负责图像尺寸调整、像素级处理和与现有RGB565转换器的格式兼容性处理。
支持智能缩放、质量优化等多种图像处理功能。
"""

import tempfile
from pathlib import Path
from dataclasses import dataclass
from typing import Tuple, Optional, List, Callable, Any
from PIL import Image, ImageFilter
import numpy as np


@dataclass
class ResizeConfig:
    """图像尺寸配置"""
    width: Optional[int] = None
    height: Optional[int] = None
    maintain_aspect_ratio: bool = True
    resize_method: str = 'LANCZOS'  # PIL重采样方法

    @property
    def has_resize(self) -> bool:
        """是否需要调整尺寸"""
        return self.width is not None or self.height is not None

    def get_target_size(self, original_size: Tuple[int, int]) -> Tuple[int, int]:
        """计算目标尺寸

        Args:
            original_size: 原始尺寸 (width, height)

        Returns:
            Tuple[int, int]: 目标尺寸
        """
        orig_width, orig_height = original_size

        if not self.has_resize:
            return original_size

        if self.maintain_aspect_ratio:
            # 保持宽高比
            if self.width is None:
                # 只指定高度
                ratio = self.height / orig_height
                target_width = int(orig_width * ratio)
                target_height = self.height
            elif self.height is None:
                # 只指定宽度
                ratio = self.width / orig_width
                target_width = self.width
                target_height = int(orig_height * ratio)
            else:
                # 两个都指定，选择较小的缩放比例
                width_ratio = self.width / orig_width
                height_ratio = self.height / orig_height
                ratio = min(width_ratio, height_ratio)
                target_width = int(orig_width * ratio)
                target_height = int(orig_height * ratio)
        else:
            # 不保持宽高比，直接拉伸
            target_width = self.width if self.width is not None else orig_width
            target_height = self.height if self.height is not None else orig_height

        return (target_width, target_height)


@dataclass
class PixelOperation:
    """像素操作基类"""
    name: str

    def apply(self, image: Image.Image) -> Image.Image:
        """应用像素操作

        Args:
            image: 输入图像

        Returns:
            Image.Image: 处理后的图像
        """
        raise NotImplementedError("子类必须实现apply方法")


class BrightnessOperation(PixelOperation):
    """亮度调整操作"""
    def __init__(self, factor: float):
        super().__init__("brightness")
        self.factor = factor

    def apply(self, image: Image.Image) -> Image.Image:
        from PIL import ImageEnhance
        enhancer = ImageEnhance.Brightness(image)
        return enhancer.enhance(self.factor)


class ContrastOperation(PixelOperation):
    """对比度调整操作"""
    def __init__(self, factor: float):
        super().__init__("contrast")
        self.factor = factor

    def apply(self, image: Image.Image) -> Image.Image:
        from PIL import ImageEnhance
        enhancer = ImageEnhance.Contrast(image)
        return enhancer.enhance(self.factor)


class SharpnessOperation(PixelOperation):
    """锐度调整操作"""
    def __init__(self, factor: float):
        super().__init__("sharpness")
        self.factor = factor

    def apply(self, image: Image.Image) -> Image.Image:
        from PIL import ImageEnhance
        enhancer = ImageEnhance.Sharpness(image)
        return enhancer.enhance(self.factor)


class FrameProcessorError(Exception):
    """帧处理错误"""
    pass


class FrameProcessor:
    """帧处理器

    负责图像尺寸调整、像素级处理和格式兼容性处理。
    优化内存使用，支持批量处理。
    """

    # 支持的重采样方法
    RESAMPLE_METHODS = {
        'NEAREST': Image.Resampling.NEAREST,
        'BILINEAR': Image.Resampling.BILINEAR,
        'BICUBIC': Image.Resampling.BICUBIC,
        'LANCZOS': Image.Resampling.LANCZOS,
        'BOX': Image.Resampling.BOX,
        'HAMMING': Image.Resampling.HAMMING
    }

    def __init__(self, resize_config: Optional[ResizeConfig] = None,
                 pixel_operations: Optional[List[PixelOperation]] = None):
        """初始化帧处理器

        Args:
            resize_config: 尺寸调整配置
            pixel_operations: 像素操作列表
        """
        self.resize_config = resize_config or ResizeConfig()
        self.pixel_operations = pixel_operations or []

    def resize_frame(self, frame: Image.Image, target_size: Tuple[int, int]) -> Image.Image:
        """调整图像尺寸

        Args:
            frame: 输入图像
            target_size: 目标尺寸

        Returns:
            Image.Image: 调整尺寸后的图像
        """
        if frame.size == target_size:
            return frame

        try:
            # 获取重采样方法
            resample_method = self.RESAMPLE_METHODS.get(
                self.resize_config.resize_method,
                Image.Resampling.LANCZOS
            )

            # 调整尺寸
            resized_frame = frame.resize(target_size, resample_method)

            return resized_frame

        except Exception as e:
            raise FrameProcessorError(f"调整图像尺寸失败: {e}")

    def apply_pixel_operations(self, frame: Image.Image) -> Image.Image:
        """应用像素级操作

        Args:
            frame: 输入图像

        Returns:
            Image.Image: 处理后的图像
        """
        result = frame.copy()

        for operation in self.pixel_operations:
            try:
                result = operation.apply(result)
            except Exception as e:
                print(f"警告: 像素操作 '{operation.name}' 失败: {e}, 跳过此操作")
                continue

        return result

    def process_frame(self, frame: Image.Image) -> Image.Image:
        """处理单帧图像

        完整的帧处理流程：
        1. 尺寸调整（如果需要）
        2. 像素级操作处理

        Args:
            frame: 输入帧

        Returns:
            Image.Image: 处理后的帧
        """
        try:
            result = frame.copy()

            # 1. 尺寸调整
            if self.resize_config.has_resize:
                target_size = self.resize_config.get_target_size(result.size)
                if target_size != result.size:
                    result = self.resize_frame(result, target_size)

            # 2. 像素级操作
            if self.pixel_operations:
                result = self.apply_pixel_operations(result)

            return result

        except Exception as e:
            raise FrameProcessorError(f"处理帧失败: {e}")

    def process_frames(self, frames: List[Image.Image]) -> List[Image.Image]:
        """批量处理帧

        Args:
            frames: 输入帧列表

        Returns:
            List[Image.Image]: 处理后的帧列表
        """
        processed_frames = []

        for i, frame in enumerate(frames):
            try:
                processed_frame = self.process_frame(frame)
                processed_frames.append(processed_frame)
            except Exception as e:
                print(f"警告: 处理第 {i} 帧失败: {e}, 使用原始帧")
                processed_frames.append(frame.copy())

        return processed_frames

    def optimize_for_embedded(self, frame: Image.Image) -> Image.Image:
        """为嵌入式系统优化图像

        Args:
            frame: 输入图像

        Returns:
            Image.Image: 优化后的图像
        """
        try:
            # 确保为RGB格式
            if frame.mode != 'RGB':
                frame = frame.convert('RGB')

            # 应用轻微的锐化以提高小屏幕显示效果
            frame = frame.filter(ImageFilter.UnsharpMask(radius=1, percent=120, threshold=3))

            return frame

        except Exception as e:
            print(f"警告: 嵌入式优化失败: {e}, 使用原始图像")
            return frame

    def save_frame_as_temp(self, frame: Image.Image, frame_index: int,
                          temp_dir: Optional[Path] = None) -> Path:
        """将帧保存为临时文件

        Args:
            frame: 输入帧
            frame_index: 帧索引
            temp_dir: 临时目录，None则使用系统默认

        Returns:
            Path: 临时文件路径
        """
        try:
            if temp_dir is None:
                temp_dir = Path(tempfile.gettempdir()) / "mp4converter"
                temp_dir.mkdir(exist_ok=True)

            # 生成文件名: frame_0001.png
            filename = f"frame_{frame_index:04d}.png"
            temp_path = temp_dir / filename

            # 确保为RGB格式并保存为PNG
            if frame.mode != 'RGB':
                frame = frame.convert('RGB')

            frame.save(temp_path, 'PNG', optimize=True)

            return temp_path

        except Exception as e:
            raise FrameProcessorError(f"保存临时文件失败: {e}")

    def save_frames_as_temp(self, frames: List[Image.Image],
                           start_index: int = 0,
                           temp_dir: Optional[Path] = None) -> List[Path]:
        """批量保存帧为临时文件

        Args:
            frames: 输入帧列表
            start_index: 起始帧索引
            temp_dir: 临时目录

        Returns:
            List[Path]: 临时文件路径列表
        """
        temp_files = []

        for i, frame in enumerate(frames):
            frame_index = start_index + i
            try:
                temp_path = self.save_frame_as_temp(frame, frame_index, temp_dir)
                temp_files.append(temp_path)
            except Exception as e:
                print(f"警告: 保存第 {frame_index} 帧为临时文件失败: {e}")
                continue

        return temp_files

    @staticmethod
    def create_resize_config(width: Optional[int] = None,
                           height: Optional[int] = None,
                           maintain_aspect_ratio: bool = True,
                           resize_method: str = 'LANCZOS') -> ResizeConfig:
        """创建尺寸调整配置

        Args:
            width: 目标宽度
            height: 目标高度
            maintain_aspect_ratio: 是否保持宽高比
            resize_method: 重采样方法

        Returns:
            ResizeConfig: 尺寸调整配置
        """
        return ResizeConfig(
            width=width,
            height=height,
            maintain_aspect_ratio=maintain_aspect_ratio,
            resize_method=resize_method
        )

    @staticmethod
    def parse_resize_string(resize_str: str) -> ResizeConfig:
        """解析尺寸调整字符串

        支持格式:
        - "120x120" - 指定宽度和高度，保持宽高比
        - "w120" - 指定宽度
        - "h120" - 指定高度

        Args:
            resize_str: 尺寸字符串

        Returns:
            ResizeConfig: 尺寸调整配置
        """
        try:
            resize_str = resize_str.lower().strip()

            if 'x' in resize_str:
                # 宽度x高度格式
                width_str, height_str = resize_str.split('x')
                width = int(width_str)
                height = int(height_str)
                return ResizeConfig(width=width, height=height)

            elif resize_str.startswith('w'):
                # 指定宽度
                width = int(resize_str[1:])
                return ResizeConfig(width=width)

            elif resize_str.startswith('h'):
                # 指定高度
                height = int(resize_str[1:])
                return ResizeConfig(height=height)

            else:
                raise ValueError(f"无法解析的尺寸格式: {resize_str}")

        except Exception as e:
            raise ValueError(f"解析尺寸字符串失败 '{resize_str}': {e}")

    @staticmethod
    def create_pixel_operations() -> List[PixelOperation]:
        """创建常用像素操作列表（可根据需要扩展）

        Returns:
            List[PixelOperation]: 像素操作列表
        """
        # 目前返回空列表，可根据需要添加预定义操作
        return []