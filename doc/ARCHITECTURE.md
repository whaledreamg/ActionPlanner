# 动作规划器架构图

## 系统架构

```
┌─────────────────────────────────────────────────────────────────┐
│                         MainWindow                              │
│  (主窗口 - 整合所有组件)                                        │
└───────────────┬─────────────────┬──────────────────────────────┘
                │                 │                  
        ┌───────▼──────┐  ┌──────▼────────┐  ┌──────────────┐
        │ Control Panel│  │  Simulation   │  │   Timeline   │
        │  (控制面板)   │  │   Widget      │  │   Editor     │
        │              │  │  (仿真视图)    │  │ (时间轴编辑) │
        └──────┬───────┘  └───────┬───────┘  └──────┬───────┘
               │                  │                  │
               │                  │                  │
        ┌──────▼──────────────────▼──────────────────▼───────┐
        │              RobotConnection                        │
        │         (TCP Socket 通信层)                         │
        └───────────────────┬─────────────────────────────────┘
                            │
                    ┌───────▼────────┐
                    │ Robot Controller│
                    │  (机械臂控制器) │
                    │  192.168.1.100 │
                    └─────────────────┘
```

## 模块通信流程

### 1. 连接建立
```
User → MainWindow → RobotControlPanel → RobotConnection
                                              ↓
                                        TCP Connect
                                              ↓
                                    Robot Controller
                                              ↓
                                        robot/list
                                              ↓
                                    Subscribe States
```

### 2. JOG控制流程
```
User Press Button
       ↓
RobotControlPanel::onJogButtonPressed()
       ↓
RobotConnection::jogStart()
       ↓
{ "ty": "robot/jogStart", "db": {...} }
       ↓
Robot Controller
       ↓
Start Timer (400ms)
       ↓
RobotConnection::jogHeartbeat()
       ↓
{ "ty": "robot/jogHeartbeat" }
       ↓
User Release Button
       ↓
Stop Timer
       ↓
RobotConnection::jogStop()
```

### 3. 状态更新流程
```
Robot Controller (50ms周期)
       ↓
Push State via TCP
       ↓
{ "ty": "publish/robot/arm0/status", "db": {...} }
       ↓
RobotConnection::onTextMessageReceived()
       ↓
emit robotStateUpdated(state)
       ↓
┌──────────────┴──────────────┐
│                              │
▼                              ▼
RobotControlPanel         SimulationWidget
updateJointDisplay()      updateJointAngles()
```

## 数据流图

```
┌────────────────┐
│  User Input    │
│  (按钮/鼠标)   │
└────────┬───────┘
         │
         ▼
┌────────────────┐      ┌─────────────────┐
│  Control       │◄────►│  Timeline       │
│  Panel         │      │  Model          │
└────────┬───────┘      └────────┬────────┘
         │                       │
         ▼                       ▼
┌────────────────┐      ┌─────────────────┐
│  Robot         │      │  Interpolation  │
│  Connection    │      │  Engine         │
└────────┬───────┘      └────────┬────────┘
         │                       │
         ▼                       ▼
┌────────────────┐      ┌─────────────────┐
│  TCP Socket    │      │  File IO        │
│  (JSON)        │      │  (JSON)         │
└────────┬───────┘      └─────────────────┘
         │
         ▼
┌────────────────┐
│  Robot         │
│  Controller    │
└────────────────┘
```

## 类关系图

```
QMainWindow
    │
    └── MainWindow
            │
            ├── RobotControlPanel (QWidget)
            │       │
            │       ├── RobotConnection (QObject)
            │       │       └── QTcpSocket
            │       │
            │       └── UI Elements
            │           ├── 7 × Joint Labels
            │           ├── 7 × JOG + Buttons
            │           ├── 7 × JOG - Buttons
            │           └── TCP Pose Display
            │
            ├── SimulationWidget (QWidget)
            │       │
            │       ├── RobotRenderer (QOpenGLWidget)
            │       │       └── URDFLoader
            │       │
            │       └── JointUpdater
            │
            └── TimelineEditor (QWidget)
                    │
                    ├── TimelineModel
                    │       └── QList<Segment>
                    │
                    ├── InterpolationEngine
                    │       ├── linearInterpolate()
                    │       └── cubicSplineInterpolate()
                    │
                    └── FileIO
                            ├── saveToJSON()
                            └── loadFromJSON()
```

## 时间轴数据结构

```
TimelineModel
    │
    ├── totalDuration: double
    │
    └── segments: QList<Segment>
            │
            ├── Segment[0]
            │   ├── label: "抓取"
            │   ├── startTime: 0.0
            │   ├── duration: 2.0
            │   ├── startJoints: JointState
            │   │   ├── j[7]: [0.0, 0.5, ...]    (左臂)
            │   │   └── jr[7]: [0.0, -0.5, ...]  (右臂)
            │   ├── endJoints: JointState
            │   │   ├── j[7]: [0.5, 1.0, ...]
            │   │   └── jr[7]: [0.5, -1.0, ...]
            │   └── interpolationType: "cubic"
            │
            ├── Segment[1]
            │   └── ...
            │
            └── Segment[n]
                └── ...
```

## 插值算法流程

```
playTimeline(model)
    │
    ├── t = 0.0
    │
    ├── while t < totalDuration:
    │   │
    │   ├── Find Segment at time t
    │   │
    │   ├── Calculate local t within segment
    │   │   localT = (t - seg.startTime) / seg.duration
    │   │
    │   ├── if interpolationType == "linear":
    │   │   result = linearInterpolate(start, end, localT)
    │   │
    │   ├── else if interpolationType == "cubic":
    │   │   result = cubicSplineInterpolate(keyframes, localT)
    │   │
    │   ├── sendJointCommand(result)
    │   │
    │   └── t += dt (4ms)
    │
    └── Done
```

## 文件格式

```json
{
  "version": "1.0",
  "totalDuration": 10.5,
  "segments": [
    {
      "label": "抓取",
      "startTime": 0.0,
      "duration": 2.0,
      "interpolation": "cubic",
      "startJoints": {
        "leftArm": [0.0, 0.5, 1.0, 0.0, 0.0, 0.0, 0.0],
        "rightArm": [0.0, -0.5, -1.0, 0.0, 0.0, 0.0, 0.0]
      },
      "endJoints": {
        "leftArm": [0.5, 1.0, 1.5, 0.0, 0.0, 0.0, 0.0],
        "rightArm": [0.5, -1.0, -1.5, 0.0, 0.0, 0.0, 0.0]
      }
    }
  ]
}
```

## 信号槽连接图

```
RobotConnection
    │
    ├── connected() ──────────► RobotControlPanel::onConnected()
    │                                 │
    │                                 └─► Enable Buttons
    │
    ├── robotStateUpdated() ──► RobotControlPanel::onRobotStateUpdated()
    │                                 │
    │                                 └─► Update Joint Labels
    │
    └── robotListReceived() ──► RobotControlPanel::onRobotListReceived()
                                      │
                                      └─► Subscribe to States

RobotControlPanel
    │
    ├── jointAnglesChanged() ──► SimulationWidget::updateJointAngles()
    │                                 │
    │                                 └─► Update 3D Model
    │
    └── currentAnglesRequested() ─► TimelineEditor::setCurrentAngles()
                                          │
                                          └─► Fill Segment Data
```

## 部署架构

```
┌─────────────────────────────────────────────────────────────┐
│  开发机器                                                   │
│  ┌─────────────────────────────────────────────────────┐  │
│  │  ActionPlanner (Qt Application)                     │  │
│  │  ├── UI (QWidgets)                                  │  │
│  │  ├── Business Logic                                 │  │
│  │  └── Network (QTcpSocket)                           │  │
│  └──────────────────────┬──────────────────────────────┘  │
│                         │ TCP/IP                           │
│                         │ JSON Protocol                    │
└─────────────────────────┼──────────────────────────────────┘
                          │
                          │ Network (192.168.1.x)
                          │
┌─────────────────────────▼──────────────────────────────────┐
│  机械臂控制器                                               │
│  ┌─────────────────────────────────────────────────────┐  │
│  │  CoMatrix Controller                                │  │
│  │  ├── EtherCAT Master                                │  │
│  │  ├── Motion Planning                                │  │
│  │  ├── Safety Monitoring                              │  │
│  │  └── Network API (TCP 9001)                         │  │
│  └──────────────────────┬──────────────────────────────┘  │
│                         │ EtherCAT                         │
└─────────────────────────┼──────────────────────────────────┘
                          │
                   ┌──────┴──────┐
                   │             │
            ┌──────▼──────┐ ┌────▼──────┐
            │  Left Arm   │ │ Right Arm │
            │  (7-axis)   │ │ (7-axis)  │
            └─────────────┘ └───────────┘
```

这个完整的架构图展示了系统的各个层次和模块之间的关系,有助于理解整体设计和后续开发。
