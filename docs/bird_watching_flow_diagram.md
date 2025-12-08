# BirdWatching模块完整运行流程示意图

## 系统架构概览

```mermaid
graph TB
    subgraph main["主程序 main.cpp"]
        A[setup] --> B[初始化硬件和系统]
        B --> C[TaskManager::initialize]
        C --> D[TaskManager::startTasks]
        D --> E[BirdWatching::initializeBirdWatching]
        E --> F[双核任务运行]
    end

    subgraph tasks["FreeRTOS双核架构"]
        F --> G[Core 0: UI Task]
        F --> H[Core 1: System Task]
        
        G --> G1[LVGL渲染]
        G --> G2[Display刷新]
        G --> G3[processTriggerRequest]
        
        H --> H1[IMU手势检测]
        H --> H2[串口命令处理]
        H --> H3[BirdManager::update]
        H --> H4[统计数据保存]
    end

    subgraph birdcore["BirdWatching 核心模块"]
        E --> I[BirdManager::initialize]
        
        I --> J[BirdAnimation初始化]
        I --> K[BirdSelector初始化]
        I --> L[BirdStatistics初始化]
        I --> M[StatsView初始化]
        I --> N[加载bird_config.csv]
        
        G3 --> O[触发小鸟动画]
        H3 --> P[定期保存统计]
    end

    subgraph interaction["交互方式"]
        Q[IMU手势] --> R[前倾1秒: 显示统计]
        Q --> S[后倾1秒: 隐藏统计]
        Q --> T[左右倾: 触发小鸟或翻页]
        
        U[串口命令] --> V[bird trigger]
        U --> W[bird stats]
        U --> X[bird list]
    end

    H1 --> Q
    H2 --> U
    O --> G1
    P --> Y[SD卡]
    L --> Y
```

## 初始化流程

```mermaid
flowchart TD
    Start([系统启动]) --> InitSerial[串口初始化 115200]
    InitSerial --> InitLogger[日志系统初始化]
    InitLogger --> InitCommands[串口命令系统初始化]
    InitCommands --> InitSD[SD卡初始化]
    
    InitSD --> InitScreen[显示屏初始化]
    InitScreen --> InitLVGL[LVGL文件系统初始化]
    InitLVGL --> InitMPU[IMU输入设备初始化]
    InitMPU --> InitRGB[RGB LED初始化]
    
    InitRGB --> CreateGUI[创建GUI界面]
    CreateGUI --> InitTaskMgr[TaskManager初始化]
    InitTaskMgr --> CreateMutex[创建LVGL互斥锁]
    
    CreateMutex --> StartTasks[启动双核任务]
    StartTasks --> Core0Task[Core 0: UI Task]
    StartTasks --> Core1Task[Core 1: System Task]
    
    Core0Task --> ShowLogo[显示Logo]
    Core1Task --> ShowLogo
    
    ShowLogo --> InitBird[BirdWatching::initialize]
    InitBird --> ScanResources[扫描小鸟资源]
    
    ScanResources --> InitAnim[BirdAnimation初始化]
    ScanResources --> InitSelector[BirdSelector初始化]
    ScanResources --> InitStats[BirdStatistics初始化]
    ScanResources --> InitStatsView[StatsView初始化]
    
    InitAnim --> LoadConfig[加载bird_config.csv]
    InitSelector --> LoadConfig
    InitStats --> LoadHistoryData[加载历史统计数据]
    
    LoadConfig --> HideLogo[关闭Logo]
    LoadHistoryData --> HideLogo
    
    HideLogo --> CheckHistory{有历史数据?}
    CheckHistory -->|是| RandomEncounter[随机显示已遇见小鸟]
    CheckHistory -->|否| TriggerFirst[触发首次小鸟]
    
    RandomEncounter --> Ready[系统就绪]
    TriggerFirst --> Ready
    
    Ready --> End([初始化完成])
```

## 双核任务运行流程

```mermaid
flowchart TD
    subgraph core0["Core 0: UI Task 200Hz"]
        UI1([UI任务循环]) --> UI2[获取LVGL互斥锁]
        UI2 --> UI3[lv_task_handler]
        UI3 --> UI4[Display::routine]
        UI4 --> UI5[processTriggerRequest]
        UI5 --> UI6{有触发请求?}
        UI6 -->|是| UI7[执行动画播放]
        UI6 -->|否| UI8[检查并隐藏小鸟信息]
        UI7 --> UI9[释放LVGL互斥锁]
        UI8 --> UI9
        UI9 --> UI10[延迟5ms]
        UI10 --> UI1
    end

    subgraph core1["Core 1: System Task 100Hz"]
        SYS1([系统任务循环]) --> SYS2[IMU::detectGesture]
        SYS2 --> SYS3{检测到手势?}
        SYS3 -->|前倾1秒| SYS4[显示统计界面]
        SYS3 -->|后倾1秒| SYS5[隐藏统计界面]
        SYS3 -->|左右倾| SYS6[触发小鸟或翻页]
        SYS3 -->|无| SYS7[处理串口命令]
        
        SYS4 --> SYS8[发送消息到UI任务]
        SYS5 --> SYS8
        SYS6 --> SYS8
        
        SYS7 --> SYS9[BirdManager::update]
        SYS8 --> SYS9
        SYS9 --> SYS10[定期保存统计]
        SYS10 --> SYS11[延迟10ms]
        SYS11 --> SYS1
    end

    SYS8 -.消息队列.-> UI5
```

## 小鸟触发流程

```mermaid
flowchart TD
    Trigger([触发请求]) --> Source{触发源}
    
    Source -->|手势触发| Gesture[IMU手势检测]
    Source -->|命令触发| Command[串口命令]
    Source -->|代码调用| API[API调用]
    
    Gesture --> SetRequest[设置触发请求]
    Command --> SetRequest
    API --> SetRequest
    
    SetRequest --> WaitUI[等待UI任务处理]
    WaitUI --> UITask[UI任务获取LVGL锁]
    
    UITask --> CheckStats{统计界面可见?}
    CheckStats -->|是| Ignore[忽略触发]
    CheckStats -->|否| CheckPlaying{正在播放?}
    
    CheckPlaying -->|是| StopAnim[停止当前动画]
    CheckPlaying -->|否| SelectBird
    
    StopAnim --> SelectBird[BirdSelector::getRandomBird]
    SelectBird --> GetInfo[获取小鸟信息]
    
    GetInfo --> CheckNew{首次遇见?}
    CheckNew -->|是| MarkNew[标记为新小鸟]
    CheckNew -->|否| GetCount[获取遇见次数]
    
    MarkNew --> LoadAnim[BirdAnimation::loadBird]
    GetCount --> LoadAnim
    
    LoadAnim --> CheckFrames{帧数大于0?}
    CheckFrames -->|是| LoadImages[加载图片序列]
    CheckFrames -->|否| Placeholder[创建彩色占位符]
    
    LoadImages --> StartLoop[startLoop循环播放]
    Placeholder --> StartLoop
    
    StartLoop --> RecordStats[记录统计数据]
    RecordStats --> ShowInfo[显示小鸟信息]
    
    ShowInfo --> SetTimer[设置5秒定时器]
    SetTimer --> Complete[触发完成]
    
    Ignore --> Complete
```

## 统计系统流程

```mermaid
flowchart TD
    StatsStart([记录遇见]) --> GetBirdId[获取小鸟ID]
    GetBirdId --> FindRecord{已有记录?}

    FindRecord -->|否| CreateNew[创建新记录]
    FindRecord -->|是| UpdateExist[更新现有记录]

    CreateNew --> SetFirst[设置首次遇见时间]
    SetFirst --> SetCount1[设置次数=1]
    SetCount1 --> SaveMem[保存到内存]

    UpdateExist --> IncrCount[次数+1]
    IncrCount --> UpdateTime[更新最后遇见时间]
    UpdateTime --> SaveMem

    SaveMem --> CheckSave{距上次保存>10秒?}
    CheckSave -->|是| SaveFile[保存到SD卡JSON]
    CheckSave -->|否| MemOnly[仅内存更新]

    SaveFile --> BuildJSON[构建JSON数据]
    BuildJSON --> WriteSD[写入/stats/bird_stats.json]
    WriteSD --> StatsEnd([统计完成])
    
    MemOnly --> StatsEnd
    
    subgraph view["统计界面显示"]
        ShowView[显示统计界面] --> LoadData[加载统计数据]
        LoadData --> CalcProgress[计算遇见进度]
        CalcProgress --> Pagination[分页显示每页5只]
        Pagination --> RenderUI[渲染LVGL界面]
    end
```

## 统计界面交互流程

```mermaid
flowchart TD
    Enter([进入统计界面]) --> StopAnim[停止当前动画]
    StopAnim --> HideInfo[隐藏小鸟信息]
    HideInfo --> CreateView[创建StatsView]
    
    CreateView --> LoadStats[加载统计数据]
    LoadStats --> CalcPages[计算总页数]
    CalcPages --> ShowPage1[显示第1页]
    
    ShowPage1 --> WaitGesture[等待手势输入]
    
    WaitGesture --> GestureCheck{手势类型}
    GestureCheck -->|左倾| PrevPage[上一页]
    GestureCheck -->|右倾| NextPage[下一页]
    GestureCheck -->|后倾1秒| Exit[退出统计界面]
    
    PrevPage --> CheckMin{当前页>1?}
    CheckMin -->|是| UpdatePrev[页码-1并刷新]
    CheckMin -->|否| FlashRed[红灯提示]
    
    NextPage --> CheckMax{当前页<总页数?}
    CheckMax -->|是| UpdateNext[页码+1并刷新]
    CheckMax -->|否| FlashRed
    
    UpdatePrev --> WaitGesture
    UpdateNext --> WaitGesture
    FlashRed --> WaitGesture
    
    Exit --> DestroyView[销毁StatsView]
    DestroyView --> ShowBird[显示一只小鸟]
    ShowBird --> CheckHistory{有历史数据?}
    CheckHistory -->|是| RandomShow[随机显示已遇见小鸟]
    CheckHistory -->|否| TriggerNew[触发新小鸟]
    
    RandomShow --> Complete([退出完成])
    TriggerNew --> Complete
```

## 小鸟选择算法流程

```mermaid
flowchart TD
    SelectStart([开始选择]) --> GetList[从BirdSelector获取列表]
    GetList --> CheckEmpty{列表为空?}
    CheckEmpty -->|是| Error[返回错误]
    CheckEmpty -->|否| CalcWeight[计算总权重]
    
    CalcWeight --> GenRandom[生成随机数 0到总权重]
    GenRandom --> Iterate[遍历小鸟列表]
    
    Iterate --> Accumulate[累加当前小鸟权重]
    Accumulate --> Compare{随机数<=累加权重?}
    
    Compare -->|是| Selected[选中该小鸟]
    Compare -->|否| HasMore{还有小鸟?}
    
    HasMore -->|是| NextBird[下一只小鸟]
    HasMore -->|否| SelectLast[选择最后一只]
    
    NextBird --> Accumulate
    SelectLast --> Selected
    
    Selected --> LogSelection[记录日志]
    LogSelection --> Return[返回BirdInfo]
    Return --> End([选择完成])
    Error --> End
```

## 手势检测处理流程

```mermaid
flowchart TD
    GestureStart([System Task检测]) --> ReadMPU[IMU::detectGesture]
    ReadMPU --> ProcessData[处理加速度计数据]

    ProcessData --> CheckForward{检测前倾?}
    CheckForward -->|是| CheckForwardTime{保持1秒?}
    CheckForwardTime -->|是| ForwardHold[GESTURE_FORWARD_HOLD]
    CheckForwardTime -->|否| CheckOther
    
    CheckForward -->|否| CheckBackward{检测后倾?}
    CheckBackward -->|是| CheckBackwardTime{保持1秒?}
    CheckBackwardTime -->|是| BackwardHold[GESTURE_BACKWARD_HOLD]
    CheckBackwardTime -->|否| CheckOther
    
    CheckBackward -->|否| CheckLeft{检测左倾?}
    CheckLeft -->|是| LeftTilt[GESTURE_LEFT_TILT]
    CheckLeft -->|否| CheckRight{检测右倾?}
    CheckRight -->|是| RightTilt[GESTURE_RIGHT_TILT]
    CheckRight -->|否| NoGesture[GESTURE_NONE]

    ForwardHold --> SendMsg[发送手势消息到UI任务]
    BackwardHold --> SendMsg
    LeftTilt --> SendMsg
    RightTilt --> SendMsg
    
    SendMsg --> ProcessInUI[UI任务处理手势]
    ProcessInUI --> CheckContext{当前上下文}
    
    CheckContext -->|统计界面| StatsContext
    CheckContext -->|主界面| MainContext
    
    subgraph StatsContext["统计界面上下文"]
        SC1[前倾1秒: 无操作]
        SC2[后倾1秒: 退出统计]
        SC3[左倾: 上一页]
        SC4[右倾: 下一页]
    end
    
    subgraph MainContext["主界面上下文"]
        MC1[前倾1秒: 显示统计]
        MC2[后倾1秒: 无操作]
        MC3[左右倾: 触发小鸟 10秒CD]
    end
    
    CheckOther --> NoGesture
    NoGesture --> GestureEnd([检测结束])
    StatsContext --> GestureEnd
    MainContext --> GestureEnd
```

## 数据持久化流程

```mermaid
flowchart TD
    PersistStart([触发保存]) --> CheckInterval{距上次>10秒?}
    CheckInterval -->|否| Skip[跳过保存]
    CheckInterval -->|是| CheckSD{SD卡就绪?}

    CheckSD -->|否| LogWarn[记录警告]
    CheckSD -->|是| GetStats[获取统计数据]

    GetStats --> BuildJSON[构建JSON对象]
    BuildJSON --> AddMeta[添加元数据]
    AddMeta --> AddTimestamp[添加时间戳]
    AddTimestamp --> AddBirds[添加小鸟数据]

    AddBirds --> OpenFile[打开stats/bird_stats.json]
    OpenFile --> FileCheck{文件打开成功?}
    
    FileCheck -->|否| CreateFile[创建新文件]
    FileCheck -->|是| WriteData[写入JSON数据]
    
    CreateFile --> WriteData
    WriteData --> CloseFile[关闭文件]
    CloseFile --> Verify{验证完整性}
    
    Verify -->|成功| UpdateTime[更新保存时间]
    Verify -->|失败| Retry{重试次数<3?}
    
    Retry -->|是| Wait[等待100ms]
    Retry -->|否| LogError[记录错误]
    
    Wait --> OpenFile
    UpdateTime --> Success[保存成功]
    
    Success --> End([持久化完成])
    LogWarn --> End
    LogError --> End
    Skip --> End
```

## 系统状态图

```mermaid
stateDiagram-v2
    [*] --> Initializing: 系统启动
    Initializing --> LogoDisplay: 显示Logo
    LogoDisplay --> ResourceScan: 扫描小鸟资源
    ResourceScan --> Ready: 资源加载完成
    
    Ready --> IdleBird: 显示小鸟
    IdleBird --> Triggering: 手势/命令触发
    Triggering --> AnimationPlaying: 播放动画
    AnimationPlaying --> IdleBird: 动画完成
    
    IdleBird --> StatsView: 前倾1秒
    StatsView --> Browsing: 浏览统计
    Browsing --> Browsing: 左右倾翻页
    Browsing --> IdleBird: 后倾1秒退出
    
    IdleBird --> ErrorState: 错误发生
    AnimationPlaying --> ErrorState: 错误发生
    StatsView --> ErrorState: 错误发生
    ErrorState --> Recovery: 错误恢复
    Recovery --> IdleBird: 恢复完成
```

## 总结

BirdWatching模块是一个完整的嵌入式观鸟应用系统，基于ESP32双核架构开发，具有以下特点：

### 核心架构
- **双核FreeRTOS架构**: Core 0处理UI渲染，Core 1处理系统逻辑和传感器
- **任务间通信**: 通过消息队列和互斥锁实现线程安全的跨核通信
- **LVGL集成**: 使用互斥锁保护所有LVGL对象访问
- **模块化设计**: BirdManager、BirdAnimation、BirdSelector、BirdStatistics独立模块

### 交互方式
- **IMU手势控制**:
  - 前倾保持1秒: 显示统计界面
  - 后倾保持1秒: 退出统计界面
  - 左右倾: 主界面触发小鸟(10秒CD) / 统计界面翻页
- **串口命令**: 
  - `bird trigger`: 触发小鸟
  - `bird stats`: 显示统计
  - `bird list`: 列出所有小鸟
  - `bird reset`: 重置统计数据

### 数据管理
- **统计追踪**: 遇见次数、首次/最后遇见时间、进度计算
- **持久化存储**: SD卡JSON格式，每10秒自动保存
- **配置文件**: bird_config.csv定义小鸟列表和权重
- **分页显示**: 统计界面每页显示5只小鸟

### 视觉反馈
- **小鸟信息显示**: 右下角显示小鸟名称和遇见次数，5秒后自动隐藏
- **新小鸟提示**: 首次遇见时特殊显示"加新{小鸟名字}！"
- **RGB LED提示**: 手势操作时LED闪烁反馈（绿色/蓝色/红色）
- **Logo过渡**: 启动时显示Logo，资源扫描完成后自动切换

### 技术特性
- **加权随机选择**: 基于权重的小鸟选择算法
- **资源管理**: 支持图片帧序列和彩色占位符
- **错误处理**: 完善的异常处理和日志记录
- **性能优化**: 看门狗配置、防抖处理、定时器管理
- **线程安全**: 跨核访问LVGL使用互斥锁保护

### 文件结构
```
src/applications/modules/bird_watching/
├── core/
│   ├── bird_watching.h/cpp      # 主接口
│   ├── bird_manager.h/cpp       # 核心管理器
│   ├── bird_animation.h/cpp     # 动画播放
│   ├── bird_selector.h/cpp      # 选择算法
│   ├── bird_stats.h/cpp         # 统计系统
│   ├── bird_types.h             # 类型定义
│   └── bird_utils.h/cpp         # 工具函数
├── ui/
│   └── stats_view.h/cpp         # 统计界面
└── screens/
    └── bird_animation_bridge.h/cpp  # GUI桥接

SD卡文件:
├── /configs/bird_config.csv     # 小鸟配置
├── /birds/{id}/                 # 小鸟图片资源
└── /stats/bird_stats.json       # 统计数据
```

该系统已完全集成到ESP32主项目中，作为独立的功能模块稳定运行，支持实时交互和数据持久化。