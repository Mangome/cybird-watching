"""
批量图片处理器
扫描目录并批量转换图片
"""

from pathlib import Path
from typing import List, Tuple, Optional, Callable
from tqdm import tqdm
import concurrent.futures
import threading

from .rgb565 import RGB565Converter


class BatchProcessor:
    """批量图片处理器"""

    def __init__(self, max_workers: int = 4):
        """
        初始化批量处理器

        Args:
            max_workers: 最大并行工作线程数
        """
        self.max_workers = max_workers
        self.lock = threading.Lock()

    @staticmethod
    def find_image_files(source_dir: Path,
                        extensions: List[str] = None) -> List[Path]:
        """
        扫描目录查找图片文件

        Args:
            source_dir: 源目录路径
            extensions: 支持的文件扩展名列表

        Returns:
            找到的图片文件路径列表
        """
        if extensions is None:
            extensions = ['.png', '.jpg', '.jpeg']

        image_files = []

        if not source_dir.exists():
            return image_files

        # 递归扫描目录
        for ext in extensions:
            # 在Windows上不区分大小写，但为了兼容性也匹配大写
            image_files.extend(source_dir.rglob(f'*{ext}'))
            image_files.extend(source_dir.rglob(f'*{ext.upper()}'))

        # 去重（使用set会丢失顺序，所以用dict）
        seen = {}
        unique_files = []
        for path in image_files:
            # 使用resolve()获取规范化的绝对路径作为去重key
            key = path.resolve()
            if key not in seen:
                seen[key] = True
                unique_files.append(path)

        return sorted(unique_files)

    @staticmethod
    def get_output_path(input_path: Path, output_dir: Path,
                       extension: str = '.bin') -> Path:
        """
        获取输出文件路径，保持相对目录结构

        Args:
            input_path: 输入文件路径
            output_dir: 输出目录
            extension: 输出文件扩展名

        Returns:
            输出文件路径
        """
        # 如果输入文件在source_dir的子目录中，保持相对路径
        relative_path = Path(input_path.name)
        return output_dir / relative_path.with_suffix(extension)

    @staticmethod
    def get_c_array_output_path(input_path: Path, output_dir: Path) -> Path:
        """
        获取C数组输出文件路径

        Args:
            input_path: 输入文件路径
            output_dir: 输出目录

        Returns:
            C数组文件路径
        """
        return output_dir / f"{input_path.stem}_rgb565.c"

    def convert_single_file(self, input_path: Path, output_path: Path,
                           max_size: Optional[Tuple[int, int]] = None) -> bool:
        """
        转换单个文件

        Args:
            input_path: 输入文件路径
            output_path: 输出文件路径
            max_size: 最大尺寸限制

        Returns:
            转换是否成功
        """
        try:
            # 确保输出目录存在
            output_path.parent.mkdir(parents=True, exist_ok=True)

            # 执行转换
            return RGB565Converter.convert_image_to_rgb565(
                input_path, output_path, max_size
            )
        except Exception as e:
            print(f"处理文件 {input_path} 时出错: {e}")
            return False

    def convert_single_to_c_array(self, input_path: Path, output_path: Path,
                                 array_name: Optional[str] = None,
                                 max_size: Optional[Tuple[int, int]] = None) -> bool:
        """
        转换单个文件为C数组

        Args:
            input_path: 输入文件路径
            output_path: 输出C文件路径
            array_name: 数组名称
            max_size: 最大尺寸限制

        Returns:
            转换是否成功
        """
        try:
            # 确保输出目录存在
            output_path.parent.mkdir(parents=True, exist_ok=True)

            # 执行转换
            return RGB565Converter.convert_image_to_c_array(
                input_path, output_path, array_name, max_size
            )
        except Exception as e:
            print(f"处理文件 {input_path} 为C数组时出错: {e}")
            return False

    def batch_convert(self, source_dir: Path, output_dir: Path,
                     max_size: Optional[Tuple[int, int]] = None,
                     output_format: str = 'binary') -> Tuple[int, int]:
        """
        批量转换图片

        Args:
            source_dir: 源目录
            output_dir: 输出目录
            max_size: 最大尺寸限制
            output_format: 输出格式 ('binary' 或 'c_array')

        Returns:
            (成功数量, 失败数量)
        """
        # 查找所有图片文件
        image_files = self.find_image_files(source_dir)

        if not image_files:
            print(f"在目录 {source_dir} 中未找到支持的图片文件")
            return 0, 0

        print(f"找到 {len(image_files)} 个图片文件")

        success_count = 0
        failed_count = 0

        # 使用进度条和线程池并行处理
        with tqdm(total=len(image_files), desc="转换进度", unit="文件") as pbar:
            with concurrent.futures.ThreadPoolExecutor(max_workers=self.max_workers) as executor:
                # 提交所有任务
                future_to_file = {}

                for input_path in image_files:
                    if output_format == 'c_array':
                        output_path = self.get_c_array_output_path(input_path, output_dir)
                        future = executor.submit(
                            self.convert_single_to_c_array,
                            input_path, output_path, None, max_size
                        )
                    else:
                        output_path = self.get_output_path(input_path, output_dir)
                        future = executor.submit(
                            self.convert_single_file,
                            input_path, output_path, max_size
                        )
                    future_to_file[future] = (input_path, output_path)

                # 处理完成的任务
                for future in concurrent.futures.as_completed(future_to_file):
                    input_path, output_path = future_to_file[future]
                    try:
                        success = future.result()
                        with self.lock:
                            if success:
                                success_count += 1
                                pbar.set_postfix({"成功": f"{success_count}/{len(image_files)}"})
                            else:
                                failed_count += 1
                                pbar.set_postfix({"失败": f"{failed_count}"})
                    except Exception as e:
                        print(f"处理 {input_path} 时发生异常: {e}")
                        with self.lock:
                            failed_count += 1

                    pbar.update(1)

        print(f"\n批量转换完成: 成功 {success_count} 个，失败 {failed_count} 个")
        return success_count, failed_count