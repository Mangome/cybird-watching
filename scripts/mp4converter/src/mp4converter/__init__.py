"""
MP4转RGB565转换器 - 将MP4视频转换为RGB565格式的帧序列

基于现有的 scripts/converter/ RGB565转换器的成功架构，为 CybirdWatching 项目设计并实现一个功能完整的 MP4 转换器。
"""

import sys
import time
from pathlib import Path
from typing import Optional

import click

from .video_decoder import VideoDecoder, VideoValidationError, VideoDecodeError
from .frame_processor import FrameProcessor
from .watermark_remover import WatermarkRemover
from .chroma_key import ChromaKeyProcessor, ChromaKeyConfig
from .batch_processor import MP4BatchProcessor, ProcessConfig
from .converter_bridge import ConverterBridge, RGB565Config


@click.group()
@click.version_option(version="1.0.0")
def cli():
    """MP4转RGB565转换器 - 将MP4视频转换为RGB565格式的帧序列

    这个工具可以将MP4视频文件转换为LVGL 7.9.1兼容的RGB565格式图像文件。
    支持帧采样、图像处理、水印去除和批量处理等功能。
    """
    pass


@cli.command()
@click.argument('video_path', type=click.Path(exists=True, path_type=Path))
@click.argument('output_dir', type=click.Path(path_type=Path))
@click.option('--frame-rate', type=int, help='采样帧率 (帧/秒), 不指定则使用原视频帧率')
@click.option('--frame-count', type=int, help='提取的总帧数')
@click.option('--resize', type=str, help='目标分辨率 (格式: WIDTHxHEIGHT 或 w120 或 h120)')
@click.option('--watermark-region', type=str, help='水印区域 (格式: X,Y,WIDTH,HEIGHT)')
@click.option('--output-format', type=click.Choice(['rgb565', 'png']),
              default='rgb565', help='输出格式')
@click.option('--rgb565-format', type=click.Choice(['binary', 'c_array']),
              default='binary', help='RGB565输出格式')
@click.option('--chroma-key', is_flag=True, help='启用自动抠图功能（去除绿底）')
@click.option('--max-width', type=int, help='最大宽度限制')
@click.option('--max-height', type=int, help='最大高度限制')
@click.option('--workers', type=int, default=4, help='并行处理线程数')
@click.option('--keep-temp', is_flag=True, help='保留临时文件用于调试')
def process(video_path: Path, output_dir: Path, frame_rate: Optional[int],
           frame_count: Optional[int], resize: Optional[str],
           watermark_region: Optional[str], output_format: str,
           rgb565_format: str, chroma_key: bool,
           max_width: Optional[int], max_height: Optional[int],
           workers: int, keep_temp: bool):
    """处理单个MP4文件

    VIDEO_PATH: 输入的MP4视频文件路径
    OUTPUT_DIR: 输出目录路径

    示例:
        mp4-converter process video.mp4 output/ --frame-rate 10 --resize 120x120

        mp4-converter process video.mp4 output/ --frame-count 20 --watermark-region "10,10,50,50"
    """
    try:
        start_time = time.time()

        print(f"开始处理视频: {video_path}")
        print(f"输出目录: {output_dir}")

        # 验证输入参数
        if frame_rate and frame_count:
            raise click.BadParameter("不能同时指定 --frame-rate 和 --frame-count")

        if frame_rate and frame_rate <= 0:
            raise click.BadParameter("帧率必须大于0")

        if frame_count and frame_count <= 0:
            raise click.BadParameter("帧数必须大于0")

        # 创建处理配置
        process_config = MP4BatchProcessor.create_process_config(
            frame_rate=frame_rate,
            frame_count=frame_count,
            resize_str=resize,
            watermark_region=watermark_region,
            rgb565_format=rgb565_format,
            max_width=max_width,
            max_height=max_height,
            output_format=output_format,
            chroma_key=chroma_key,
            workers=workers,
            continue_on_error=False
        )

        # 创建批量处理器
        processor = MP4BatchProcessor(max_workers=workers)

        # 处理单个视频
        result = processor.process_single_video(video_path, output_dir, process_config)

        # 输出结果
        total_time = time.time() - start_time

        print(f"\n{'='*50}")
        print("处理完成:")
        if result.success:
            print(f"  状态: 成功")
            print(f"  帧数: {result.frame_count}")
            print(f"  RGB565文件: {len(result.rgb565_files)}")
            if result.video_info:
                print(f"  视频信息: {result.video_info.width}x{result.video_info.height}, "
                      f"{result.video_info.fps:.2f}fps, {result.video_info.duration:.1f}秒")

            print(f"\n生成的文件:")
            for rgb565_file in result.rgb565_files:
                print(f"  - {rgb565_file}")
        else:
            print(f"  状态: 失败")
            print(f"  错误: {result.error_message}")
            sys.exit(1)

        print(f"  总处理时间: {total_time:.1f}秒")
        print(f"{'='*50}")

    except Exception as e:
        print(f"错误: {e}")
        sys.exit(1)


@cli.command()
@click.argument('input_dir', type=click.Path(exists=True, path_type=Path))
@click.argument('output_dir', type=click.Path(path_type=Path))
@click.option('--frame-rate', type=int, help='采样帧率 (帧/秒)')
@click.option('--frame-count', type=int, help='提取的总帧数')
@click.option('--resize', type=str, help='目标分辨率 (格式: WIDTHxHEIGHT 或 w120 或 h120)')
@click.option('--watermark-region', type=str, help='水印区域 (格式: X,Y,WIDTH,HEIGHT)')
@click.option('--output-format', type=click.Choice(['rgb565', 'png']),
              default='rgb565', help='输出格式')
@click.option('--rgb565-format', type=click.Choice(['binary', 'c_array']),
              default='binary', help='RGB565输出格式')
@click.option('--max-width', type=int, help='最大宽度限制')
@click.option('--max-height', type=int, help='最大高度限制')
@click.option('--workers', type=int, default=4, help='并行处理线程数')
@click.option('--continue-on-error', is_flag=True, help='遇到错误时继续处理其他文件')
@click.option('--keep-temp', is_flag=True, help='保留临时文件用于调试')
def batch(input_dir: Path, output_dir: Path, frame_rate: Optional[int],
         frame_count: Optional[int], resize: Optional[str],
         watermark_region: Optional[str], output_format: str,
         rgb565_format: str, max_width: Optional[int], max_height: Optional[int],
         workers: int, continue_on_error: bool, keep_temp: bool):
    """批量处理目录中的所有MP4文件

    INPUT_DIR: 包含MP4文件的输入目录
    OUTPUT_DIR: 输出目录路径

    示例:
        mp4-converter batch videos/ output/ --frame-rate 5 --resize 64x64 --workers 8

        mp4-converter batch videos/ output/ --frame-count 10 --output-format png --continue-on-error

        mp4-converter batch videos/ output/ --output-format rgb565 --rgb565-format c_array
    """
    try:
        print(f"开始批量处理:")
        print(f"  输入目录: {input_dir}")
        print(f"  输出目录: {output_dir}")

        # 验证输入参数
        if frame_rate and frame_count:
            raise click.BadParameter("不能同时指定 --frame-rate 和 --frame-count")

        # 创建处理配置
        process_config = MP4BatchProcessor.create_process_config(
            frame_rate=frame_rate,
            frame_count=frame_count,
            resize_str=resize,
            watermark_region=watermark_region,
            rgb565_format=rgb565_format,
            max_width=max_width,
            max_height=max_height,
            output_format=output_format,
            workers=workers,
            continue_on_error=continue_on_error
        )

        # 创建批量处理器
        processor = MP4BatchProcessor(max_workers=workers)

        # 批量处理
        result = processor.process_videos(input_dir, output_dir, process_config)

        # 根据结果设置退出码
        if result.failed_videos > 0 and not continue_on_error:
            sys.exit(1)

    except Exception as e:
        print(f"错误: {e}")
        sys.exit(1)


@cli.command()
@click.argument('video_path', type=click.Path(exists=True, path_type=Path))
def info(video_path: Path):
    """显示视频文件信息

    VIDEO_PATH: 要分析的视频文件路径

    示例:
        mp4-converter info video.mp4
    """
    try:
        decoder = VideoDecoder()

        # 验证文件
        decoder.validate_video_file(video_path)

        # 获取视频信息
        video_info = decoder.get_video_info(video_path)

        # 显示信息
        print(f"视频信息: {video_path.name}")
        print(f"{'='*40}")
        print(f"分辨率: {video_info.width}x{video_info.height}")
        print(f"帧率: {video_info.fps:.2f} fps")
        print(f"总帧数: {video_info.frame_count}")
        print(f"时长: {video_info.duration:.1f} 秒")
        print(f"编解码器: {video_info.codec}")
        print(f"宽高比: {video_info.aspect_ratio:.2f}")
        print(f"文件大小: {video_info.file_size / (1024*1024):.1f} MB")

        # 显示采样建议
        print(f"\n采样建议:")
        if video_info.duration <= 10:
            suggested_frames = min(10, video_info.frame_count)
            print(f"  短视频 (<10s): 建议提取 {suggested_frames} 帧")
        elif video_info.duration <= 30:
            suggested_frames = min(20, video_info.frame_count)
            print(f"  中等视频 (10-30s): 建议提取 {suggested_frames} 帧")
        else:
            suggested_rate = min(5, video_info.fps)
            print(f"  长视频 (>30s): 建议使用 {suggested_rate:.1f} fps")

        print(f"{'='*40}")

    except VideoValidationError as e:
        print(f"文件验证错误: {e}")
        sys.exit(1)
    except VideoDecodeError as e:
        print(f"视频解码错误: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"错误: {e}")
        sys.exit(1)


@cli.command()
@click.argument('converter_path', type=click.Path(exists=True, path_type=Path))
def test_converter(converter_path: Path):
    """测试与现有converter的集成

    CONVERTER_PATH: converter目录路径

    示例:
        mp4-converter test-converter ../converter/
    """
    try:
        print(f"测试converter集成: {converter_path}")

        # 创建桥接器
        with ConverterBridge(converter_path=converter_path) as bridge:
            print("[OK] Converter桥接器创建成功")

            # 测试配置
            config = RGB565Config(format='binary', max_width=64, max_height=64)
            print("[OK] RGB565配置创建成功")

            # 测试参数转换
            args = config.to_converter_args()
            print(f"[OK] Converter参数生成成功: {' '.join(args)}")

        print("[OK] 所有测试通过")
        print("converter集成正常工作")

    except Exception as e:
        print(f"[FAIL] 测试失败: {e}")
        sys.exit(1)


def main():
    """主入口函数"""
    try:
        cli()
    except KeyboardInterrupt:
        print("\n操作被用户中断")
        sys.exit(1)
    except Exception as e:
        print(f"未预期的错误: {e}")
        sys.exit(1)


if __name__ == '__main__':
    main()
