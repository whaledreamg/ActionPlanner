# 动作规划器实现总结

## 已完成的工作

我已经按照设计方案完成了动作规划器的基础框架搭建和核心功能实现。以下是详细的完成情况：

### 1. 项目结构 ✅

创建了完整的Qt项目结构，包括:
- `ActionPlanner.pro` - qmake项目配置文件
- `src/main.cpp` - 程序入口
- 完整的模块化目录结构 (ui/, robot/, simulation/, timeline/, utils/)
- 所有必需的头文件(.h)和实现文件(.cpp)
- UI文件(.ui)用于Qt Designer

### 2. WebSocket通信模块 ✅

**RobotConnection类** (`src/robot/RobotConnection.{h,cpp}`)
- 完整的WebSocket连接管理
- JSON消息封装和解析
- 支持的命令:
  - `robot/list` - 枚举机器人
  - `robot/enable` / `robot/disable` - 使能控制
  - `robot/status` - 状态查询
  - `robot/jogStart` / `jogHeartbeat` / `jogStop` - JOG点动
  - `robotState/sub` - 状态订阅
- 信号-槽机制处理异步响应
- 自动消息ID管理

### 3. 机械臂控制面板 ✅

**RobotControlPanel** (`src/ui/RobotControlPanel.{h,cpp,ui}`)

#### 功能特性:
- **连接管理**: 
  - 可配置IP地址和端口
  - 连接状态显示
  - 自动枚举左右臂
  
- **使能控制**:
  - 独立的左臂/右臂使能/失能按钮
  - 状态实时反馈

- **关节信息显示**:
  - 7个关节的实时角度显示(rad)
  - TCP位姿显示(x, y, z)
  - 动态创建的Widget布局

- **JOG微调**:
  - 每个关节配备 +/- 按钮
  - 按住连续点动，松开停止
  - 400ms心跳维持
  - 支持关节空间和笛卡尔空间(接口已实现)

- **当前角度获取**:
  - 一键读取机器人当前关节角度
  - 信号传递到时间轴编辑器

#### UI布局:
- Tab页切换左右臂
- GroupBox分组显示关节和TCP信息
- QFormLayout整齐排列

### 4. 主窗口框架 ✅

**MainWindow** (`src/ui/MainWindow.{h,cpp,ui}`)
- 三栏布局: 控制面板 | 仿真视图 | 时间轴编辑器
- 信号连接各模块
- 菜单栏: 文件/打开/保存/退出
- 状态栏显示

### 5. 时间轴数据模型 ✅

**TimelineModel** (`src/timeline/TimelineModel.{h,cpp}`)

#### 数据结构:
```cpp
struct JointState {
    double j[7];   // 左臂关节
    double jr[7];  // 右臂关节
};

struct Segment {
    QString label;              // 片段标签
    double startTime;           // 开始时间
    double duration;            // 持续时间
    JointState startJoints;     // 起始关节角度
    JointState endJoints;       // 结束关节角度
    QString interpolationType;  // 插值类型
};
```

#### 功能:
- 添加/删除/查询片段
- 自动计算总时长
- 支持多段动作序列

### 6. 插值引擎 ✅

**InterpolationEngine** (`src/timeline/InterpolationEngine.{h,cpp}`)

#### 已实现算法:
1. **线性插值** (`linearInterpolate`)
   - 简单快速
   - 适用于直线运动
   
2. **三次样条插值** (`cubicSplineInterpolate`)
   - 平滑连续
   - 适用于曲线轨迹
   - 当前实现为分段线性(待完善为真正的样条)

#### 使用示例:
```cpp
JointState start, end;
// ... 设置起点和终点
double t = 0.5; // 中点
JointState mid = InterpolationEngine::linearInterpolate(start, end, t);
```

### 7. 文件IO模块 ✅

**FileIO** (`src/utils/FileIO.{h,cpp}`)
- JSON格式保存/加载
- 使用Qt的QJsonDocument
- 支持格式化输出(缩进)

### 8. 其他支持模块 ✅

- **RobotStateManager**: 机器人状态管理(基础框架)
- **RealtimeController**: 实时控制接口(待实现具体逻辑)
- **URDFLoader**: URDF解析器(接口定义完成)
- **RobotRenderer**: 3D渲染器(OpenGL框架搭建完成)
- **SimulationWidget**: 仿真视图容器
- **TimelineEditor**: 时间轴编辑器UI框架

## 编译和运行

### 依赖安装 (Ubuntu):
```bash
sudo apt install qt5-default qtwebsockets5-dev libtinyxml2-dev libeigen3-dev
```

### 编译:
```bash
cd /home/gxh/workspace/project/ActionPlanner
qmake ActionPlanner.pro
make
```

### 运行:
```bash
./ActionPlanner
```

## 核心技术实现细节

### 1. WebSocket异步通信

使用Qt的QWebSocket实现非阻塞通信:

```cpp
// 发送命令
void RobotConnection::sendCommand(const QString& ty, const QJsonObject& db, int id) {
    QJsonObject message;
    message["id"] = id;
    message["ty"] = ty;
    message["db"] = db;
    
    QString jsonString = QJsonDocument(message).toJson(QJsonDocument::Compact);
    m_webSocket->sendTextMessage(jsonString);
}

// 接收响应
void RobotConnection::onTextMessageReceived(const QString& message) {
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    QJsonObject obj = doc.object();
    
    if (obj["ty"].toString().startsWith("publish/")) {
        emit robotStateUpdated(obj["db"].toObject());
    } else {
        emit responseReceived(obj);
    }
}
```

### 2. JOG连续点动实现

使用QTimer实现心跳维持:

```cpp
// 按下按钮 - 启动点动
void RobotControlPanel::onJogButtonPressed(const QString& arm, int joint, int direction) {
    m_connection->jogStart(armName, "joint", QString::number(joint), direction, 0.1);
    m_activeJogArm = armName;
    m_jogHeartbeatTimer->start(400); // 400ms周期心跳
}

// 定时器触发 - 发送心跳
connect(m_jogHeartbeatTimer, &QTimer::timeout, [this]() {
    if (!m_activeJogArm.isEmpty()) {
        m_connection->jogHeartbeat(m_activeJogArm);
    }
});

// 松开按钮 - 停止点动
void RobotControlPanel::onJogButtonReleased() {
    m_jogHeartbeatTimer->stop();
    m_connection->jogStop(m_activeJogArm);
    m_activeJogArm.clear();
}
```

### 3. 状态订阅与UI更新

```cpp
// 订阅机器人状态
m_connection->subscribeToState(m_leftArmName, 50); // 50ms周期

// 接收状态更新
void RobotControlPanel::onRobotStateUpdated(const QJsonObject& state) {
    QJsonArray jointsArray = state["joints"].toArray();
    QVector<double> joints;
    for (const QJsonValue& val : jointsArray) {
        joints.append(val.toDouble());
    }
    
    updateJointDisplay("left", joints);
    emit jointAnglesChanged(m_leftJoints, m_rightJoints);
}
```

### 4. 动态UI创建

为7个关节动态创建控制Widget:

```cpp
for (int i = 0; i < 7; ++i) {
    QWidget* jointWidget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(jointWidget);
    
    QLabel* label = new QLabel(QString("J%1: 0.000").arg(i+1));
    QPushButton* minusBtn = new QPushButton("-");
    QPushButton* plusBtn = new QPushButton("+");
    
    connect(plusBtn, &QPushButton::pressed, [this, i]() {
        onJogButtonPressed("left", i, 1);
    });
    
    layout->addWidget(label);
    layout->addWidget(minusBtn);
    layout->addWidget(plusBtn);
    leftLayout->addWidget(jointWidget);
}
```

## 待完善功能

### 优先级 1: URDF 3D渲染

**需要实现**:
1. TinyXML2解析URDF文件
2. 提取link和joint信息
3. 加载STL mesh文件
4. OpenGL渲染链
5. 正运动学计算更新位姿

**参考实现**:
```cpp
bool URDFLoader::loadURDF(const QString& filePath) {
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(filePath.toStdString().c_str()) != tinyxml2::XML_SUCCESS) {
        return false;
    }
    
    // 解析<link>标签
    tinyxml2::XMLElement* linkElem = doc.FirstChildElement("robot")->FirstChildElement("link");
    while (linkElem) {
        Link link;
        link.name = linkElem->Attribute("name");
        // 解析visual/geometry/mesh
        // ...
        m_links.append(link);
        linkElem = linkElem->NextSiblingElement("link");
    }
    
    return true;
}
```

### 优先级 2: 时间轴拖动编辑

**需要实现**:
1. 自定义QWidget绘制时间轴
2. 鼠标事件处理(拖动/调整)
3. 片段选中/高亮
4. 右键菜单
5. 片段编辑对话框

**关键代码**:
```cpp
void TimelineEditor::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    
    // 绘制时间刻度
    for (int t = 0; t <= m_totalDuration; t++) {
        int x = t * m_pixelsPerSecond;
        painter.drawLine(x, 0, x, height());
    }
    
    // 绘制片段
    for (const Segment& seg : m_segments) {
        int x = seg.startTime * m_pixelsPerSecond;
        int w = seg.duration * m_pixelsPerSecond;
        painter.drawRect(x, 50, w, 100);
        painter.drawText(x, 70, seg.label);
    }
}
```

### 优先级 3: 实时控制播放

**需要实现**:
1. 调用 `realtime/startControl` 入控
2. 按控制周期(4ms)采样轨迹
3. UDP或WebSocket发送目标
4. QElapsedTimer精确计时
5. 调用 `realtime/stopControl` 退控

**参考实现**:
```cpp
void RealtimeController::playTimeline(const TimelineModel& model) {
    // 入控
    QJsonObject config;
    config["profile"] = "default";
    config["cartesianIkPath"] = "none";
    m_connection->sendCommand("realtime/startControl", config);
    
    // 播放循环
    QElapsedTimer timer;
    timer.start();
    double dt = 0.004; // 4ms
    
    while (timer.elapsed() / 1000.0 < model.totalDuration()) {
        double t = timer.elapsed() / 1000.0;
        JointState target = sampleTrajectory(model, t);
        
        // 发送目标
        QJsonObject joints;
        joints["leftArm"] = QJsonArray::fromVector(QVector<double>(target.j, target.j + 7));
        m_connection->sendJointCommand(joints);
        
        QThread::msleep(4);
    }
    
    // 退控
    m_connection->sendCommand("realtime/stopControl", QJsonObject());
}
```

## 测试建议

### 1. 连接测试
```bash
# 确保控制器在线
ping 192.168.1.100

# 启动程序
./ActionPlanner

# 点击连接，观察日志输出
```

### 2. JOG测试
1. 连接成功后点击"使能"
2. 按住关节+/-按钮
3. 观察关节角度变化
4. 松开应立即停止

### 3. 状态订阅测试
- 连接后自动订阅
- 观察关节角度和TCP位姿实时更新(50ms周期)

## 扩展建议

### 1. 增强功能
- 添加笛卡尔空间JOG
- 实现速度倍率调节
- 添加轨迹录制功能
- 支持多个动作序列文件管理

### 2. 用户体验
- 添加连接状态指示灯
- 实现异常提示对话框
- 添加操作历史记录(Undo/Redo)
- 快捷键支持

### 3. 性能优化
- WebSocket消息队列管理
- UI刷新限流(避免高频更新卡顿)
- 轨迹预计算缓存
- 多线程渲染

## 总结

本项目已完成基础框架和核心功能的实现，可以进行编译和基本测试。主要完成的模块包括:

1. ✅ 完整的Qt项目结构
2. ✅ WebSocket通信和机器人控制
3. ✅ 图形化控制面板
4. ✅ JOG点动微调
5. ✅ 状态实时监控
6. ✅ 时间轴数据模型
7. ✅ 插值算法实现
8. ✅ JSON文件读写

下一步需要重点完善:
- URDF解析和3D渲染
- 时间轴图形化编辑
- 实时控制播放功能

整体架构清晰，模块化设计良好，后续扩展方便。
