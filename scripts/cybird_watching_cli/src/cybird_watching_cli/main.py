"""
CybirdWatching CLI 主入口点
"""
import asyncio
import sys
import argparse
from pathlib import Path

from .config.settings import CybirdWatchingConfig, ConfigManager
from .core.connection import SerialConnectionManager
from .core.response_handler import CommandResponseHandler
from .core.command_executor import CommandExecutor
from .ui.console import ConsoleInterface
from .utils.exceptions import CybirdCLIError, ConnectionError


class CybirdWatchingCLI:
    """主应用程序类"""

    def __init__(self, cli_args=None):
        self.cli_args = cli_args or {}
        self.config = self._load_configuration()
        self.connection = SerialConnectionManager(self.config.serial)
        self.response_handler = CommandResponseHandler(self.config.response)
        self.command_executor = CommandExecutor(
            self.connection,
            self.response_handler,
            self.config
        )
        self.console = ConsoleInterface(self.config)
        self.running = False

    def _load_configuration(self) -> CybirdWatchingConfig:
        """加载配置"""
        try:
            config_manager = ConfigManager()
            config = config_manager.load_config()

            # 覆盖命令行参数
            if self.cli_args.get('port'):
                config.serial.port = self.cli_args['port']
            if self.cli_args.get('baudrate'):
                config.serial.baudrate = self.cli_args['baudrate']

            return config

        except Exception as e:
            print(f"配置加载失败，使用默认配置: {e}")
            return CybirdWatchingConfig()

    async def run_interactive(self, initial_command: str = None) -> None:
        """运行交互式模式"""
        self.running = True
        self.console.show_welcome()

        # 尝试连接设备
        await self._connect_device()

        # 如果有初始命令，执行它
        if initial_command:
            await self._execute_and_display(initial_command)

        # 主命令循环
        await self._command_loop()

    async def _connect_device(self) -> None:
        """连接设备"""
        try:
            self.console.show_info(f"正在连接到设备 ({self.config.serial.port})...")
            connected = await self.connection.connect()

            if connected:
                self.console.show_connection_status(True, self.config.serial.port)
            else:
                self.console.show_error("设备连接失败，输入 'reconnect' 重试")
        except Exception as e:
            self.console.show_error(f"连接错误: {str(e)}")

    async def _command_loop(self) -> None:
        """主命令循环"""
        while self.running:
            try:
                prompt = self.console.get_prompt(self.connection.is_connected)
                user_input = self.console.get_user_input(prompt)

                if not user_input:
                    continue

                user_input = user_input.strip().lower()

                # 处理本地命令
                if await self._handle_local_command(user_input):
                    continue

                # 处理设备命令
                await self._execute_and_display(user_input)

            except KeyboardInterrupt:
                self.console.show_info("\n收到中断信号，正在退出...")
                break
            except EOFError:
                break
            except Exception as e:
                self.console.show_error(f"输入错误: {str(e)}")

    async def _handle_local_command(self, command: str) -> bool:
        """处理本地命令，返回True表示已处理"""
        if command in ['quit', 'exit']:
            self.running = False
            return True
        elif command == 'help':
            self.console.show_help()
            return True
        elif command == 'test':
            await self._execute_test_command()
            return True
        elif command == 'reconnect':
            await self._reconnect_device()
            return True
        elif command == 'cls':
            self.console.clear_screen()
            return True
        elif command == 'info':
            await self._show_device_info()
            return True

        return False

    async def _execute_and_display(self, command: str) -> None:
        """执行命令并显示结果"""
        self.console.show_command_sent(command)

        try:
            result = await self.command_executor.execute_command_with_validation(command)
            self.console.show_command_result(result)
        except Exception as e:
            self.console.show_error(f"命令执行失败: {str(e)}")

    async def _execute_test_command(self) -> None:
        """执行测试命令"""
        self.console.show_info("测试模式：发送简单ping命令（无响应标记）...")
        self.console.show_command_sent("help")

        try:
            result = await self.command_executor.execute_test_command()
            self.console.show_command_result(result)
        except Exception as e:
            self.console.show_error(f"测试命令失败: {str(e)}")

    async def _reconnect_device(self) -> None:
        """重新连接设备"""
        self.console.show_info("正在断开连接...")
        self.connection.disconnect()

        await self._connect_device()

    async def _show_device_info(self) -> None:
        """显示设备信息"""
        try:
            device_info = self.connection.get_connection_info()
            self.console.show_device_info(device_info)
        except Exception as e:
            self.console.show_error(f"获取设备信息失败: {str(e)}")

    async def send_single_command(self, command: str) -> None:
        """发送单个命令（非交互模式）"""
        try:
            # 连接设备
            await self._connect_device()

            if not self.connection.is_connected:
                print("错误: 无法连接到设备")
                sys.exit(1)

            # 执行命令
            self.console.show_command_sent(command)
            result = await self.command_executor.execute_command_with_validation(command)
            self.console.show_command_result(result)

            # 根据结果设置退出码
            if not result.success:
                sys.exit(1)

        except Exception as e:
            self.console.show_error(f"命令执行失败: {str(e)}")
            sys.exit(1)

    def cleanup(self) -> None:
        """清理资源"""
        try:
            # 保存命令历史
            self.console.save_history()
        except Exception:
            pass
        
        try:
            self.connection.disconnect()
        except Exception:
            pass


def create_parser() -> argparse.ArgumentParser:
    """创建命令行参数解析器"""
    parser = argparse.ArgumentParser(
        description="CybirdWatching CLI - ESP32设备命令行工具",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  %(prog)s                          # 交互式模式
  %(prog)s --port COM4              # 指定端口
  %(prog)s --baudrate 9600          # 指定波特率
  %(prog)s send "log"               # 发送单个命令
  %(prog)s send "status"            # 发送状态查询命令
        """
    )

    parser.add_argument(
        '--port', '-p',
        default='COM3',
        help='串口端口 (默认: COM3)'
    )

    parser.add_argument(
        '--baudrate', '-b',
        type=int,
        default=115200,
        help='波特率 (默认: 115200)'
    )

    parser.add_argument(
        '--version', '-v',
        action='version',
        version='CybirdWatching CLI v1.1.0'
    )

    subparsers = parser.add_subparsers(dest='command', help='子命令')

    # send命令
    send_parser = subparsers.add_parser('send', help='发送单个命令到设备')
    send_parser.add_argument('device_command', help='要发送的命令')

    return parser


async def main():
    """主函数"""
    parser = create_parser()
    args = parser.parse_args()

    # 创建CLI实例
    cli = CybirdWatchingCLI({
        'port': args.port,
        'baudrate': args.baudrate
    })

    try:
        if args.command == 'send':
            # 单命令模式
            await cli.send_single_command(args.device_command)
        else:
            # 交互式模式
            await cli.run_interactive()

    except KeyboardInterrupt:
        print("\n程序被用户中断")
    except Exception as e:
        print(f"程序执行错误: {str(e)}")
        sys.exit(1)
    finally:
        cli.cleanup()


def main_sync():
    """同步入口点（用于scripts设置）"""
    asyncio.run(main())


if __name__ == "__main__":
    main_sync()