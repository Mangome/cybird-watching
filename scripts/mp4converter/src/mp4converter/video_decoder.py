"""
视频解码和帧提取核心模块

负责MP4视频文件的解析、元数据提取和帧采样功能。
支持均匀分布采样算法，确保在视频时长内均匀提取指定数量的帧。
"""

import os
import tempfile
from pathlib import Path
from dataclasses import dataclass
from typing import List, Optional, Iterator, Tuple
import cv2
import numpy as np
from PIL import Image
import ffmpeg


@dataclass
class VideoInfo:
    """视频信息数据类"""
    width: int
    height: int
    fps: float
    frame_count: int
    duration: float  # 秒
    codec: str
    file_size: int  # 字节

    @property
    def aspect_ratio(self) -> float:
        """计算视频宽高比"""
        return self.width / self.height if self.height > 0 else 1.0


@dataclass
class Frame:
    """视频帧数据类"""
    index: int
    timestamp: float  # 秒
    image: Image.Image  # PIL Image对象

    @property
    def size(self) -> Tuple[int, int]:
        """获取帧尺寸"""
        return self.image.size


class VideoValidationError(Exception):
    """视频验证错误"""
    pass


class VideoDecodeError(Exception):
    """视频解码错误"""
    pass


class VideoDecoder:
    """视频解码器

    负责MP4视频的解析、验证和帧提取。
    支持均匀分布的帧采样算法，优化内存使用。
    """

    # 支持的视频格式
    SUPPORTED_FORMATS = {'.mp4', '.avi', '.mov', '.mkv', '.wmv'}

    # 最大文件大小限制 (100MB)
    MAX_FILE_SIZE = 100 * 1024 * 1024

    # 最大分辨率限制 (1080p)
    MAX_RESOLUTION = (1920, 1080)

    def __init__(self, max_memory_mb: int = 512):
        """初始化视频解码器

        Args:
            max_memory_mb: 最大内存使用限制（MB）
        """
        self.max_memory_mb = max_memory_mb
        self.max_memory_bytes = max_memory_mb * 1024 * 1024

    def validate_video_file(self, video_path: Path) -> None:
        """验证视频文件

        Args:
            video_path: 视频文件路径

        Raises:
            VideoValidationError: 文件验证失败
        """
        if not video_path.exists():
            raise VideoValidationError(f"视频文件不存在: {video_path}")

        if not video_path.is_file():
            raise VideoValidationError(f"路径不是文件: {video_path}")

        # 检查文件扩展名
        if video_path.suffix.lower() not in self.SUPPORTED_FORMATS:
            raise VideoValidationError(
                f"不支持的视频格式: {video_path.suffix}, "
                f"支持的格式: {', '.join(self.SUPPORTED_FORMATS)}"
            )

        # 检查文件大小
        file_size = video_path.stat().st_size
        if file_size > self.MAX_FILE_SIZE:
            raise VideoValidationError(
                f"视频文件过大: {file_size / (1024*1024):.1f}MB, "
                f"最大支持: {self.MAX_FILE_SIZE / (1024*1024)}MB"
            )

    def get_video_info(self, video_path: Path) -> VideoInfo:
        """获取视频信息

        Args:
            video_path: 视频文件路径

        Returns:
            VideoInfo: 视频信息对象

        Raises:
            VideoDecodeError: 解码失败
        """
        # 默认使用OpenCV获取视频信息
        try:
            cap = cv2.VideoCapture(str(video_path))

            if not cap.isOpened():
                cap.release()
                raise VideoDecodeError("无法打开视频文件")

            try:
                # 获取基本信息
                width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
                height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
                fps = cap.get(cv2.CAP_PROP_FPS)
                frame_count = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))

                # 获取时长
                duration = frame_count / fps if fps > 0 else 0.0

                # 检查分辨率限制
                if width > self.MAX_RESOLUTION[0] or height > self.MAX_RESOLUTION[1]:
                    raise VideoValidationError(
                        f"视频分辨率过高: {width}x{height}, "
                        f"最大支持: {self.MAX_RESOLUTION[0]}x{self.MAX_RESOLUTION[1]}"
                    )

                # 获取文件大小
                file_size = video_path.stat().st_size

                return VideoInfo(
                    width=width,
                    height=height,
                    fps=fps if fps > 0 else 25.0,
                    frame_count=frame_count,
                    duration=duration,
                    codec="opencv_detected",  # OpenCV不直接提供编解码器信息
                    file_size=file_size
                )

            finally:
                cap.release()

        except Exception as e:
            if isinstance(e, (VideoDecodeError, VideoValidationError)):
                raise
            raise VideoDecodeError(f"获取视频信息失败: {e}")

    def uniform_frame_sampling(self, video_info: VideoInfo, target_frames: Optional[int]) -> List[int]:
        """均匀分布帧采样算法

        根据用户选择的策略，在视频时长内均匀提取指定数量的帧。
        例如：100帧视频提取10帧，则提取第0、11、22、33、...、99帧。

        Args:
            video_info: 视频信息
            target_frames: 目标帧数，None表示使用原始帧率

        Returns:
            List[int]: 要提取的帧索引列表
        """
        if target_frames is None:
            # 使用视频原始帧率，即提取所有帧
            return list(range(video_info.frame_count))

        if target_frames >= video_info.frame_count:
            # 目标帧数不超过视频总帧数
            return list(range(video_info.frame_count))

        if target_frames <= 0:
            raise ValueError("目标帧数必须大于0")

        # 均匀分布采样
        interval = video_info.frame_count / target_frames
        frame_indices = []

        for i in range(target_frames):
            frame_index = int(i * interval)
            # 确保不超过视频总帧数
            frame_index = min(frame_index, video_info.frame_count - 1)
            frame_indices.append(frame_index)

        return frame_indices

    def extract_frame(self, video_path: Path, frame_index: int) -> Frame:
        """提取单帧

        Args:
            video_path: 视频文件路径
            frame_index: 帧索引

        Returns:
            Frame: 提取的帧对象

        Raises:
            VideoDecodeError: 解码失败
        """
        try:
            # 使用OpenCV读取视频
            cap = cv2.VideoCapture(str(video_path))

            if not cap.isOpened():
                raise VideoDecodeError(f"无法打开视频文件: {video_path}")

            # 跳转到指定帧
            cap.set(cv2.CAP_PROP_POS_FRAMES, frame_index)

            # 读取帧
            ret, frame = cap.read()

            cap.release()

            if not ret:
                raise VideoDecodeError(f"无法读取第 {frame_index} 帧")

            # OpenCV使用BGR格式，转换为RGB
            frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

            # 转换为PIL Image
            image = Image.fromarray(frame_rgb)

            # 计算时间戳
            video_info = self.get_video_info(video_path)
            timestamp = frame_index / video_info.fps if video_info.fps > 0 else 0.0

            return Frame(
                index=frame_index,
                timestamp=timestamp,
                image=image
            )

        except Exception as e:
            if isinstance(e, VideoDecodeError):
                raise
            raise VideoDecodeError(f"提取帧 {frame_index} 失败: {e}")

    def extract_frames(self, video_path: Path, frame_indices: List[int]) -> Iterator[Frame]:
        """批量提取帧

        Args:
            video_path: 视频文件路径
            frame_indices: 要提取的帧索引列表

        Yields:
            Frame: 提取的帧对象

        Raises:
            VideoDecodeError: 解码失败
        """
        try:
            # 一次性打开视频文件，提高效率
            cap = cv2.VideoCapture(str(video_path))

            if not cap.isOpened():
                raise VideoDecodeError(f"无法打开视频文件: {video_path}")

            # 获取视频信息用于计算时间戳
            video_info = self.get_video_info(video_path)

            for frame_index in frame_indices:
                # 跳转到指定帧
                cap.set(cv2.CAP_PROP_POS_FRAMES, frame_index)

                # 读取帧
                ret, frame = cap.read()

                if not ret:
                    print(f"警告: 无法读取第 {frame_index} 帧，跳过")
                    continue

                # OpenCV使用BGR格式，转换为RGB
                frame_rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

                # 转换为PIL Image
                image = Image.fromarray(frame_rgb)

                # 计算时间戳
                timestamp = frame_index / video_info.fps if video_info.fps > 0 else 0.0

                yield Frame(
                    index=frame_index,
                    timestamp=timestamp,
                    image=image
                )

            cap.release()

        except Exception as e:
            if isinstance(e, VideoDecodeError):
                raise
            raise VideoDecodeError(f"批量提取帧失败: {e}")

    def can_load_frames(self, frame_size: Tuple[int, int], frame_count: int) -> bool:
        """检查是否有足够内存加载帧

        Args:
            frame_size: 帧尺寸 (width, height)
            frame_count: 帧数量

        Returns:
            bool: 是否有足够内存
        """
        # 估算每帧内存使用量（RGB格式，每像素3字节）
        bytes_per_pixel = 3
        frame_memory = frame_size[0] * frame_size[1] * bytes_per_pixel
        total_memory = frame_memory * frame_count

        return total_memory < self.max_memory_bytes

    def get_optimal_batch_size(self, frame_size: Tuple[int, int],
                             target_frame_count: int) -> int:
        """获取最优批处理大小

        Args:
            frame_size: 帧尺寸
            target_frame_count: 目标帧数

        Returns:
            int: 最优批处理大小
        """
        # 预留一些内存给其他进程使用
        available_memory = self.max_memory_bytes * 0.8

        # 计算单帧内存使用量
        bytes_per_pixel = 3
        frame_memory = frame_size[0] * frame_size[1] * bytes_per_pixel

        # 计算可以同时处理的帧数
        max_frames_in_memory = int(available_memory / frame_memory)

        # 确保至少处理1帧，最多处理目标帧数
        batch_size = max(1, min(max_frames_in_memory, target_frame_count))

        return batch_size