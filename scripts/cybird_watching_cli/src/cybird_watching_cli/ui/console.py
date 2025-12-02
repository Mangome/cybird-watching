"""
控制台界面模块
"""
from typing import Optional
import sys
import os

# 尝试导入 readline 以支持历史命令功能
try:
    import readline
    READLINE_AVAILABLE = True
except ImportError:
    # Windows 上尝试使用 pyreadline3
    try:
        import pyreadline3 as readline
        READLINE_AVAILABLE = True
    except ImportError:
        READLINE_AVAILABLE = False
        readline = None

try:
    from rich.console import Console
    from rich.text import Text
    from rich.panel import Panel
    from rich.prompt import Prompt
    from rich.table import Table
    from rich import box
    RICH_AVAILABLE = True
except ImportError:
    RICH_AVAILABLE = False
    Console = None

from ..config.settings import CybirdWatchingConfig
from ..core.command_executor import CommandResult


class OutputFormatter:
    """输出格式化器"""

    def __init__(self, enable_colors: bool = True):
        self.enable_colors = enable_colors and RICH_AVAILABLE
        self.console = Console() if self.enable_colors else None

    def format_success(self, message: str) -> str:
        """格式化成功消息"""
        if self.enable_colors:
            return f"[green]✓[/green] {message}"
        return f"✓ {message}"

    def format_error(self, message: str) -> str:
        """格式化错误消息"""
        if self.enable_colors:
            return f"[red]✗[/red] {message}"
        return f"✗ {message}"

    def format_warning(self, message: str) -> str:
        """格式化警告消息"""
        if self.enable_colors:
            return f"[yellow]⚠[/yellow] {message}"
        return f"⚠ {message}"

    def format_info(self, message: str) -> str:
        """格式化信息消息"""
        if self.enable_colors:
            return f"[blue]ℹ[/blue] {message}"
        return f"ℹ {message}"

    def format_command(self, command: str) -> str:
        """格式化命令显示"""
        if self.enable_colors:
            return f"[yellow]发送:[/yellow] {command}"
        return f"发送: {command}"

    def format_response(self, response: str) -> str:
        """格式化响应显示"""
        if self.enable_colors:
            return f"[cyan]响应:[/cyan]\n{response}"
        return f"响应:\n{response}"

    def print_colored(self, text: str, color: Optional[str] = None) -> None:
        """打印彩色文本"""
        if self.enable_colors and color:
            self.console.print(text, style=color)
        elif self.enable_colors:
            self.console.print(text)
        else:
            print(text)

    def print_success(self, message: str) -> None:
        """打印成功消息"""
        self.print_colored(self.format_success(message), "green")

    def print_error(self, message: str) -> None:
        """打印错误消息"""
        self.print_colored(self.format_error(message), "red")

    def print_warning(self, message: str) -> None:
        """打印警告消息"""
        self.print_colored(self.format_warning(message), "yellow")

    def print_info(self, message: str) -> None:
        """打印信息消息"""
        self.print_colored(self.format_info(message), "blue")

    def print_command_result(self, result: CommandResult) -> None:
        """打印命令执行结果"""
        if result.success:
            if result.response:
                self.print_colored(self.format_response(result.response))
            if result.execution_time > 0:
                self.print_info(f"执行时间: {result.execution_time:.2f}秒")
        else:
            self.print_error(result.error or "命令执行失败")


class ConsoleInterface:
    """控制台界面"""

    def __init__(self, config: CybirdWatchingConfig):
        self.config = config
        self.formatter = OutputFormatter(config.ui.enable_colors)
        self.console = Console() if RICH_AVAILABLE else None
        self._setup_readline()

    def _setup_readline(self) -> None:
        """设置 readline 以支持历史命令"""
        if not READLINE_AVAILABLE:
            return

        # 设置历史文件路径
        home_dir = os.path.expanduser("~")
        history_file = os.path.join(home_dir, ".cybird_watching_history")
        self.history_file = history_file

        # 设置历史记录最大长度
        readline.set_history_length(1000)

        # 尝试加载历史记录
        try:
            if os.path.exists(history_file):
                readline.read_history_file(history_file)
        except Exception:
            # 忽略历史文件读取错误
            pass

        # 设置补全和编辑模式
        try:
            # 启用 Tab 补全（可选）
            readline.parse_and_bind("tab: complete")
            
            # 在某些系统上，可能需要设置编辑模式
            if sys.platform.startswith('linux') or sys.platform == 'darwin':
                readline.parse_and_bind("set editing-mode emacs")
        except Exception:
            pass

    def save_history(self) -> None:
        """保存历史命令到文件"""
        if not READLINE_AVAILABLE:
            return

        try:
            readline.write_history_file(self.history_file)
        except Exception:
            # 忽略历史文件写入错误
            pass

    def show_welcome(self) -> None:
        """显示欢迎信息"""
        welcome_text = "CybirdWatching CLI - 跨平台命令行工具 v1.1"

        if self.console:
            panel = Panel(
                f"[bold green]{welcome_text}[/bold green]",
                title="欢迎使用",
                border_style="green"
            )
            self.console.print(panel)
        else:
            print(f"\n{welcome_text}")
            print("=" * len(welcome_text))

    def show_help(self) -> None:
        """显示帮助信息"""
        help_text = """
=== CybirdWatching CLI 帮助 ===

设备命令 (发送到ESP32设备):

📊 日志管理:
  log              - 显示最后20行日志
  log clear        - 清空日志文件
  log size         - 显示日志文件大小
  log lines N      - 显示最后N行日志 (1-500)
  log cat/export   - 显示完整日志文件内容

🔧 系统状态:
  status           - 显示系统状态
  clear            - 清除设备终端屏幕
  help             - 显示设备帮助

📁 文件管理:
  tree [path] [levels] - 显示SD卡目录树

🐦 观鸟功能:
  bird trigger     - 手动触发小鸟动画
  bird list        - 显示可用小鸟列表
  bird stats       - 显示观鸟统计信息
  bird status      - 显示观鸟系统状态
  bird help        - 显示观鸟命令帮助

本地命令:
  help             - 显示此CLI帮助
  test             - 测试基本通信（无响应标记）
  quit, exit       - 退出程序
  reconnect        - 重新连接设备
  cls              - 清除此终端屏幕
  info             - 显示设备连接信息

示例:
  [ON] CybirdWatching> log          # 显示设备日志
  [ON] CybirdWatching> log cat      # 显示完整日志
  [ON] CybirdWatching> status       # 显示设备状态
  [ON] CybirdWatching> log lines 20 # 显示最后20行日志
  [ON] CybirdWatching> tree         # 显示SD卡目录树
  [ON] CybirdWatching> tree /config 2 # 显示config目录，2层深度
  [ON] CybirdWatching> bird list    # 查看可用小鸟列表
  [ON] CybirdWatching> bird trigger # 手动触发小鸟
  [ON] CybirdWatching> bird stats   # 查看观鸟统计
  [ON] CybirdWatching> bird status  # 查看观鸟系统状态
  [ON] CybirdWatching> test         # 测试通信
        """

        if self.console:
            panel = Panel(
                help_text.strip(),
                title="帮助信息",
                border_style="blue"
            )
            self.console.print(panel)
        else:
            print(help_text)

    def get_user_input(self, prompt: str) -> str:
        """获取用户输入（支持历史命令）"""
        try:
            # 使用 input() 配合 readline，自动支持上下键历史命令
            user_input = input(prompt)
            user_input = user_input.strip()
            
            # 只记录非空命令到历史
            if READLINE_AVAILABLE and user_input:
                # readline 会自动将输入添加到历史记录
                # 但我们可以手动保存以确保持久化
                pass
            
            return user_input
        except (EOFError, KeyboardInterrupt):
            return "quit"

    def format_output(self, content: str, output_type: str = "response") -> str:
        """格式化输出内容"""
        if output_type == "command":
            return self.formatter.format_command(content)
        elif output_type == "response":
            return self.formatter.format_response(content)
        elif output_type == "success":
            return self.formatter.format_success(content)
        elif output_type == "error":
            return self.formatter.format_error(content)
        elif output_type == "warning":
            return self.formatter.format_warning(content)
        elif output_type == "info":
            return self.formatter.format_info(content)
        else:
            return content

    def show_connection_status(self, is_connected: bool, port: str) -> None:
        """显示连接状态"""
        status = "ON" if is_connected else "OFF"
        status_text = f"设备状态: {status}"

        if is_connected:
            status_text += f" (端口: {port})"

        if is_connected:
            self.formatter.print_success(status_text)
        else:
            self.formatter.print_error(status_text)

    def show_command_sent(self, command: str) -> None:
        """显示命令发送提示"""
        self.formatter.print_colored(self.formatter.format_command(command))

    def show_command_result(self, result: CommandResult) -> None:
        """显示命令执行结果"""
        self.formatter.print_command_result(result)

    def clear_screen(self) -> None:
        """清屏"""
        import os
        os.system('cls' if os.name == 'nt' else 'clear')

    def show_error(self, message: str) -> None:
        """显示错误消息"""
        self.formatter.print_error(message)

    def show_info(self, message: str) -> None:
        """显示信息消息"""
        self.formatter.print_info(message)

    def show_warning(self, message: str) -> None:
        """显示警告消息"""
        self.formatter.print_warning(message)

    def get_prompt(self, is_connected: bool) -> str:
        """获取命令提示符"""
        status = "[ON]" if is_connected else "[OFF]"
        return self.config.ui.prompt_template.format(status=status)

    def show_device_info(self, device_info: dict) -> None:
        """显示设备信息"""
        if not self.console:
            print(f"设备信息: {device_info}")
            return

        table = Table(title="设备信息", box=box.ROUNDED)
        table.add_column("属性", style="cyan", no_wrap=True)
        table.add_column("值", style="white")

        table.add_row("端口", device_info.get('port', 'Unknown'))
        table.add_row("波特率", str(device_info.get('baudrate', 'Unknown')))
        table.add_row("连接状态", "已连接" if device_info.get('is_connected') else "未连接")
        table.add_row("端口开放", "是" if device_info.get('is_open') else "否")

        if device_info.get('is_open'):
            table.add_row("CD", str(device_info.get('cd', 'N/A')))
            table.add_row("DSR", str(device_info.get('dsr', 'N/A')))
            table.add_row("CTS", str(device_info.get('cts', 'N/A')))
            table.add_row("RI", str(device_info.get('ri', 'N/A')))

        self.console.print(table)