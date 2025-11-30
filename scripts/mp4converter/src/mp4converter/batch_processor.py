"""
批量处理和并行化模块

负责多文件处理、进度跟踪、错误处理和恢复、并发控制。
支持多线程并行处理，提高转换效率。
"""

import time
import threading
from pathlib import Path
from typing import List, Dict, Any, Optional, Callable, Iterator, Tuple
from dataclasses import dataclass
from concurrent.futures import ThreadPoolExecutor, as_completed
from tqdm import tqdm

from .video_decoder import VideoDecoder, VideoInfo, Frame
from .frame_processor import FrameProcessor, ResizeConfig
from .watermark_remover import WatermarkRemover, Rectangle
from .chroma_key import ChromaKeyProcessor, ChromaKeyConfig
from .converter_bridge import ConverterBridge, RGB565Config, ConversionResult


@dataclass
class ProcessConfig:
    """处理配置数据类"""
    frame_rate: Optional[int] = None  # 采样帧率
    frame_count: Optional[int] = None  # 提取的总帧数
    resize_config: Optional[ResizeConfig] = None  # 尺寸调整配置
    watermark_region: Optional[str] = None  # 水印区域字符串
    rgb565_config: Optional[RGB565Config] = None  # RGB565转换配置
    chroma_key_config: Optional[ChromaKeyConfig] = None  # 抠图配置
    output_format: str = 'rgb565'  # 输出格式 ('rgb565' 或 'png')
    workers: int = 4  # 并行处理线程数
    continue_on_error: bool = False  # 遇到错误时是否继续处理其他文件


@dataclass
class ProcessResult:
    """处理结果数据类"""
    video_path: Path
    success: bool
    rgb565_files: List[Path]
    error_message: Optional[str] = None
    processing_time: Optional[float] = None
    frame_count: int = 0
    video_info: Optional[VideoInfo] = None


@dataclass
class BatchProcessResult:
    """批量处理结果数据类"""
    total_videos: int
    successful_videos: int
    failed_videos: int
    total_frames: int
    total_rgb565_files: int
    total_processing_time: float
    results: List[ProcessResult]

    @property
    def success_rate(self) -> float:
        """成功率"""
        return self.successful_videos / self.total_videos if self.total_videos > 0 else 0.0

    @property
    def average_processing_time(self) -> float:
        """平均处理时间"""
        return self.total_processing_time / self.total_videos if self.total_videos > 0 else 0.0


class BatchProcessorError(Exception):
    """批量处理错误"""
    pass


class MP4BatchProcessor:
    """MP4批量处理器

    支持多文件处理、并行处理、进度跟踪和错误恢复。
    优化内存使用，提供详细的处理统计信息。
    """

    def __init__(self, max_workers: int = 4, temp_dir: Optional[Path] = None):
        """初始化批量处理器

        Args:
            max_workers: 最大工作线程数
            temp_dir: 临时目录路径
        """
        self.max_workers = max_workers
        self.temp_dir = temp_dir
        self.lock = threading.Lock()  # 线程安全的状态更新

        # 初始化组件
        self.video_decoder = VideoDecoder()
        self.watermark_remover = WatermarkRemover.create_default_remover()
        self.chroma_key_processor = ChromaKeyProcessor()

    def find_video_files(self, input_dir: Path) -> List[Path]:
        """查找目录中的所有MP4文件

        Args:
            input_dir: 输入目录

        Returns:
            List[Path]: 视频文件路径列表
        """
        try:
            video_files = []

            # 支持的视频扩展名
            video_extensions = {'.mp4', '.avi', '.mov', '.mkv', '.wmv'}

            for ext in video_extensions:
                video_files.extend(input_dir.glob(f"*{ext}"))
                video_files.extend(input_dir.glob(f"*{ext.upper()}"))

            # 按文件名排序
            video_files.sort(key=lambda x: x.name.lower())

            return video_files

        except Exception as e:
            raise BatchProcessorError(f"查找视频文件失败: {e}")

    def process_single_video(self, video_path: Path, output_dir: Path,
                           config: ProcessConfig) -> ProcessResult:
        """处理单个视频文件

        完整的处理流程：
        1. 验证视频文件
        2. 获取视频信息
        3. 提取帧
        4. 处理帧（尺寸调整、水印去除等）
        5. 转换为RGB565格式

        Args:
            video_path: 视频文件路径
            output_dir: 输出目录
            config: 处理配置

        Returns:
            ProcessResult: 处理结果
        """
        start_time = time.time()
        video_name = video_path.stem

        try:
            print(f"\n开始处理视频: {video_path.name}")

            # 1. 验证视频文件
            self.video_decoder.validate_video_file(video_path)

            # 2. 获取视频信息
            video_info = self.video_decoder.get_video_info(video_path)
            print(f"视频信息: {video_info.width}x{video_info.height}, "
                  f"{video_info.fps:.2f}fps, {video_info.frame_count}帧, "
                  f"{video_info.duration:.1f}秒")

            # 3. 确定帧采样策略
            if config.frame_count:
                frame_indices = self.video_decoder.uniform_frame_sampling(
                    video_info, config.frame_count
                )
            else:
                frame_indices = self.video_decoder.uniform_frame_sampling(
                    video_info, config.frame_rate
                )

            print(f"将提取 {len(frame_indices)} 帧")

            # 4. 初始化帧处理器
            frame_processor = FrameProcessor(
                resize_config=config.resize_config
            )

            # 5. 批量处理帧
            processed_frames = []
            for i, frame_index in enumerate(frame_indices):
                try:
                    # 提取帧
                    frame = self.video_decoder.extract_frame(video_path, frame_index)

                    # 图像处理
                    processed_frame = frame_processor.process_frame(frame.image)

                    # 水印处理
                    if config.watermark_region:
                        processed_frame = self.watermark_remover.remove_watermark_from_region_string(
                            processed_frame, config.watermark_region
                        )

                    processed_frames.append(processed_frame)

                    if (i + 1) % 10 == 0 or i == len(frame_indices) - 1:
                        print(f"已处理 {i + 1}/{len(frame_indices)} 帧")

                except Exception as e:
                    print(f"处理第 {frame_index} 帧失败: {e}")
                    if not config.continue_on_error:
                        raise

            if not processed_frames:
                raise BatchProcessorError("没有成功处理的帧")

            print(f"成功处理 {len(processed_frames)} 帧")

            # 6. 抠图处理
            if config.chroma_key_config:
                print("应用自动抠图处理...")
                processed_frames = self.chroma_key_processor.process_images(processed_frames, auto_detect=True)

            # 7. 转换输出格式
            with ConverterBridge() as converter_bridge:
                conversion_result = converter_bridge.convert_frames_to_output(
                    frames=processed_frames,
                    video_name=video_name,
                    output_dir=output_dir / video_name,
                    config=config.rgb565_config
                )

                if not conversion_result.success:
                    raise BatchProcessorError(f"转换失败: {conversion_result.error_message}")

                processing_time = time.time() - start_time

                output_format_name = "RGB565" if config.output_format == 'rgb565' else "PNG"
                print(f"完成转换: 生成 {len(conversion_result.rgb565_files)} 个{output_format_name}文件, "
                      f"耗时 {processing_time:.1f}秒")

                return ProcessResult(
                    video_path=video_path,
                    success=True,
                    rgb565_files=conversion_result.rgb565_files,
                    processing_time=processing_time,
                    frame_count=len(conversion_result.rgb565_files),
                    video_info=video_info
                )

        except Exception as e:
            processing_time = time.time() - start_time
            error_message = str(e)

            print(f"处理视频失败 {video_path.name}: {error_message}")

            return ProcessResult(
                video_path=video_path,
                success=False,
                rgb565_files=[],
                error_message=error_message,
                processing_time=processing_time,
                frame_count=0
            )

    def process_videos_parallel(self, video_paths: List[Path], output_dir: Path,
                              config: ProcessConfig) -> BatchProcessResult:
        """并行处理多个视频文件

        Args:
            video_paths: 视频文件路径列表
            output_dir: 输出目录
            config: 处理配置

        Returns:
            BatchProcessResult: 批量处理结果
        """
        if not video_paths:
            return BatchProcessResult(
                total_videos=0,
                successful_videos=0,
                failed_videos=0,
                total_frames=0,
                total_rgb565_files=0,
                total_processing_time=0.0,
                results=[]
            )

        print(f"开始批量处理 {len(video_paths)} 个视频文件，使用 {config.workers} 个并行线程")

        start_time = time.time()
        results = []

        # 创建输出目录
        output_dir.mkdir(parents=True, exist_ok=True)

        # 使用线程池并行处理
        with ThreadPoolExecutor(max_workers=config.workers) as executor:
            # 提交所有任务
            future_to_video = {
                executor.submit(self.process_single_video, video_path, output_dir, config): video_path
                for video_path in video_paths
            }

            # 使用进度条显示处理进度
            with tqdm(total=len(video_paths), desc="处理视频") as pbar:
                for future in as_completed(future_to_video):
                    video_path = future_to_video[future]
                    try:
                        result = future.result()
                        results.append(result)

                        if result.success:
                            pbar.set_postfix({
                                '成功': sum(1 for r in results if r.success),
                                '失败': sum(1 for r in results if not r.success)
                            })
                        else:
                            if not config.continue_on_error:
                                print(f"处理失败，停止批量处理: {result.error_message}")
                                # 取消剩余任务
                                for f in future_to_video:
                                    f.cancel()
                                break

                    except Exception as e:
                        error_result = ProcessResult(
                            video_path=video_path,
                            success=False,
                            rgb565_files=[],
                            error_message=str(e),
                            processing_time=0.0
                        )
                        results.append(error_result)

                        if not config.continue_on_error:
                            print(f"处理异常，停止批量处理: {e}")
                            # 取消剩余任务
                            for f in future_to_video:
                                f.cancel()
                            break

                    pbar.update(1)

        # 计算统计信息
        total_processing_time = time.time() - start_time
        successful_videos = sum(1 for r in results if r.success)
        failed_videos = len(results) - successful_videos
        total_frames = sum(r.frame_count for r in results if r.success)
        total_rgb565_files = sum(len(r.rgb565_files) for r in results if r.success)

        return BatchProcessResult(
            total_videos=len(video_paths),
            successful_videos=successful_videos,
            failed_videos=failed_videos,
            total_frames=total_frames,
            total_rgb565_files=total_rgb565_files,
            total_processing_time=total_processing_time,
            results=results
        )

    def process_videos(self, input_dir: Path, output_dir: Path,
                      config: ProcessConfig) -> BatchProcessResult:
        """批量处理目录中的视频文件

        Args:
            input_dir: 输入目录
            output_dir: 输出目录
            config: 处理配置

        Returns:
            BatchProcessResult: 批量处理结果
        """
        try:
            # 1. 查找视频文件
            video_files = self.find_video_files(input_dir)

            if not video_files:
                print(f"在目录 {input_dir} 中未找到视频文件")
                return BatchProcessResult(
                    total_videos=0,
                    successful_videos=0,
                    failed_videos=0,
                    total_frames=0,
                    total_rgb565_files=0,
                    total_processing_time=0.0,
                    results=[]
                )

            print(f"找到 {len(video_files)} 个视频文件:")
            for video_file in video_files:
                print(f"  - {video_file.name}")

            # 2. 并行处理
            result = self.process_videos_parallel(video_files, output_dir, config)

            # 3. 输出处理结果摘要
            print(f"\n{'='*60}")
            print("批量处理完成:")
            print(f"  总视频数: {result.total_videos}")
            print(f"  成功处理: {result.successful_videos}")
            print(f"  处理失败: {result.failed_videos}")
            print(f"  成功率: {result.success_rate:.1%}")
            print(f"  总帧数: {result.total_frames}")
            print(f"  总RGB565文件: {result.total_rgb565_files}")
            print(f"  总处理时间: {result.total_processing_time:.1f}秒")
            print(f"  平均处理时间: {result.average_processing_time:.1f}秒/视频")
            print(f"{'='*60}")

            # 4. 输出失败的视频
            if result.failed_videos > 0:
                print(f"\n失败的视频:")
                for process_result in result.results:
                    if not process_result.success:
                        print(f"  - {process_result.video_path.name}: {process_result.error_message}")

            return result

        except Exception as e:
            raise BatchProcessorError(f"批量处理失败: {e}")

    @staticmethod
    def create_process_config(frame_rate: Optional[int] = None,
                            frame_count: Optional[int] = None,
                            resize_str: Optional[str] = None,
                            watermark_region: Optional[str] = None,
                            rgb565_format: str = 'binary',
                            max_width: Optional[int] = None,
                            max_height: Optional[int] = None,
                            output_format: str = 'rgb565',
                            chroma_key: bool = False,
                            workers: int = 4,
                            continue_on_error: bool = False) -> ProcessConfig:
        """创建处理配置

        Args:
            frame_rate: 采样帧率
            frame_count: 提取的总帧数
            resize_str: 尺寸调整字符串
            watermark_region: 水印区域字符串
            rgb565_format: RGB565输出格式
            max_width: 最大宽度
            max_height: 最大高度
            output_format: 输出格式 ('rgb565' 或 'png')
            chroma_key: 是否启用抠图功能
            workers: 并行工作线程数
            continue_on_error: 遇到错误时是否继续

        Returns:
            ProcessConfig: 处理配置
        """
        # 解析尺寸配置
        resize_config = None
        if resize_str:
            resize_config = FrameProcessor.parse_resize_string(resize_str)

        # 创建RGB565配置
        rgb565_enabled = (output_format == 'rgb565')
        rgb565_config = RGB565Config(
            format=rgb565_format,
            max_width=max_width,
            max_height=max_height,
            enabled=rgb565_enabled
        )

        # 创建抠图配置
        chroma_key_config = None
        if chroma_key:
            chroma_key_config = ChromaKeyProcessor.create_green_screen_config()

        return ProcessConfig(
            frame_rate=frame_rate,
            frame_count=frame_count,
            resize_config=resize_config,
            watermark_region=watermark_region,
            rgb565_config=rgb565_config,
            chroma_key_config=chroma_key_config,
            output_format=output_format,
            workers=workers,
            continue_on_error=continue_on_error
        )