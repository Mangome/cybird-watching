# 小白快速上手指南

本文档适合了解基本的PC操作，知道怎么安装软件的非开发者用户。

## 1. 环境准备

### 1.1 硬件

淘宝上搜索关键字“HoloCubic”、“稚晖君”就能找到很多出售整机的商家，各家都大差不差。购买预装了[HoloCubic_AIO](https://github.com/ClimbSnail/HoloCubic_AIO)固件的版本即可。

最便捷的方式就是征求商家帮你把本项目的固件刷新到硬件上后再发货，就不需要自己动手了。


### 1.2 开发环境

下载源码：[压缩包分享地址](https://pan.quark.cn/s/db0359f6f5f9)，或者直接从[github](https://github.com/Mangome/cybird-watching)上下载，解压到不包含中文的路径下。

安装Python，下载地址：[清华大学源 Python 3.14.2](https://mirrors.tuna.tsinghua.edu.cn/python/3.14.2/python-3.14.2.exe)

Python 安装完成后运行源码`scripts/`目录下的`init.bat`进行初始化。

### 1.3 准备 SD 卡

将 SD 卡插入电脑（自备读卡器），确保 SD 卡已格式化为 FAT32 格式。
将 `resources/` 目录下的所有内容拷贝到 SD 卡根目录，不需要包含 `resources/` 目录本身。

SD 卡内容结构：
```
SD卡根目录/
├── birds/              # 小鸟图片资源
│   ├── 1001/          # 小鸟 ID 目录
│   ├── 1002/
│   └── ...
├── configs/           # 配置文件
│   └── bird_config.csv
└── static/            # 静态资源
    └── logo.bin       # 启动 Logo
```

## 2 编译和烧录固件

**确定自己的硬件 COM 端口**。将 USB 连接电脑和 ESP32 硬件，打开设备管理器（开始菜单搜索`设备管理器`）。

![image.png](https://static-1317922524.cos.ap-guangzhou.myqcloud.com/static/202512092334859.png)

如截图所示，当前演示设备的端口是`COM3`。修改源码中的 `platformio.ini`，将`upload_port`端口改为你的电脑的设备端口。

进入 `scripts/` 目录，双击运行 `upload_and_monitor.bat`，等待烧录完成。

初次执行在国内网络可能需要比较长时间的下载：
![image.png](https://static-1317922524.cos.ap-guangzhou.myqcloud.com/static/202512100113486.png)

下载完成后会进入到烧录过程：
![image.png](https://static-1317922524.cos.ap-guangzhou.myqcloud.com/static/202512092336035.png)

烧录过程中硬件会息屏，烧录完成后显示 `Cybird Watching` Logo 就烧录成功了。
![image.png](https://static-1317922524.cos.ap-guangzhou.myqcloud.com/static/202512092341687.png)

