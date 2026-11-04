# 机器人连接后状态更新修复

## 问题

机器人连接成功后，UI界面没有显示状态更新（关节角度、使能状态等）。

## 原因分析

对比参考实现 `/home/gxh/workspace/project/RobotControl/communication/armwebsocket.{h,cpp}`，发现以下问题：

1. **连接后没有自动订阅** - 参考实现在connected后500ms自动调用`subscribeAll()`
2. **没有正确解析推送消息** - 推送消息格式是 `{"ty": "publish/...", "topic": "...", "db": {...}}`
3. **订阅消息格式错误** - 订阅字段必须放顶层，不能在db内
4. **没有区分左右臂** - 状态更新时需要根据`name`字段判断是哪个臂

## 修复内容

### 1. RobotConnection::onConnected() - 连接后自动订阅

```cpp
void RobotConnection::onConnected()
{
    m_isConnected = true;
    emit connected();
    
    // 连接成功后延迟500ms再订阅（等待握手稳定）
    QTimer::singleShot(500, this, [this]() {
        // 先获取机器人列表以确定真实臂名
        listRobots();
        
        // 订阅左右臂状态推送（50ms推送间隔）
        QTimer::singleShot(300, this, [this]() {
            subscribeToState("LeftArm", 50);
            subscribeToState("RightArm", 50);
            
            // 主动拉取一次初始状态
            getRobotStatus("LeftArm");
            getRobotStatus("RightArm");
        });
    });
}
```

### 2. RobotConnection::subscribeToState() - 正确的订阅格式

**关键**：订阅消息的`topics`和`tc`字段必须放在顶层，不能包在`db`内！

```cpp
void RobotConnection::subscribeToState(const QString& robotName, int tc)
{
    qDebug() << "Subscribing to" << robotName << "status with tc =" << tc << "ms";
    
    // 订阅消息格式（订阅字段放顶层，不在db内！）
    QJsonObject sub;
    sub["id"] = getNextId();
    sub["ty"] = "robotState/sub";
    
    // topics可以是字符串或数组
    QJsonArray topics;
    topics.append(QString("robot/%1/status").arg(robotName));
    sub["topics"] = topics;
    sub["tc"] = tc;  // 推送间隔（毫秒）
    
    QJsonDocument doc(sub);
    QString jsonStr = doc.toJson(QJsonDocument::Compact);
    jsonStr += "\n";
    m_webSocket->write(jsonStr.toUtf8());
    m_webSocket->flush();
}
```

### 3. RobotConnection::onTextMessageReceived() - 解析推送消息

推送消息格式：
```json
{
    "ty": "publish/robotState/sub",
    "topic": "robot/LeftArm/status",
    "db": {
        "name": "LeftArm",
        "joints": [0.1, 0.2, ...],
        "cartesian": {...},
        "enabled": true,
        ...
    }
}
```

```cpp
void RobotConnection::onTextMessageReceived(const QString& message)
{
    // 可能一次收到多行JSON
    QStringList lines = message.split('\n', Qt::SkipEmptyParts);
    
    for (const QString& line : lines) {
        if (line.trimmed().isEmpty()) continue;
        
        QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
        if (!doc.isObject()) continue;
        
        QJsonObject obj = doc.object();
        
        // 检查是否是推送消息（状态更新）
        if (obj.contains("ty")) {
            QString ty = obj["ty"].toString();
            
            if (ty.startsWith("publish/")) {
                // 这是订阅推送消息
                QString topic = obj.value("topic").toString();
                QJsonObject db = obj.value("db").toObject();
                
                // 解析机器人状态
                if (topic.contains("/status")) {
                    emit robotStateUpdated(db);
                }
                continue;
            }
        }
        
        // 处理响应消息...
    }
}
```

### 4. RobotControlPanel::onRobotStateUpdated() - 区分左右臂

```cpp
void RobotControlPanel::onRobotStateUpdated(const QJsonObject& state)
{
    QString robotName = state.value("name").toString();
    
    // 提取关节角度
    if (state.contains("joints")) {
        QJsonArray jointsArray = state["joints"].toArray();
        QVector<double> joints;
        for (const QJsonValue& val : jointsArray) {
            joints.append(val.toDouble());
        }
        
        if (joints.size() >= 7) {
            // 根据机器人名称判断左右臂
            bool isLeftArm = robotName.contains("Left", Qt::CaseInsensitive) || 
                            robotName.contains("left") ||
                            robotName == m_leftArmName;
            
            if (isLeftArm) {
                m_leftJoints = joints;
                updateJointDisplay("left", joints);
                emit jointAnglesChanged("left", joints);  // 发送给仿真系统
            } else {
                m_rightJoints = joints;
                updateJointDisplay("right", joints);
                emit jointAnglesChanged("right", joints);
            }
        }
        
        // 提取TCP位姿
        if (state.contains("cartesian")) {
            QJsonObject cart = state["cartesian"].toObject();
            QJsonArray pos = cart["pos"].toArray();
            if (pos.size() >= 3) {
                double x = pos[0].toDouble();
                double y = pos[1].toDouble();
                double z = pos[2].toDouble();
                bool isLeftArm = ...;
                updateTCPDisplay(isLeftArm ? "left" : "right", x, y, z);
            }
        }
    }
    
    // 提取使能状态
    if (state.contains("enabled")) {
        bool enabled = state["enabled"].toBool();
        bool isLeftArm = ...;
        
        if (isLeftArm) {
            ui->btnEnableLeft->setEnabled(!enabled);
            ui->btnDisableLeft->setEnabled(enabled);
        } else {
            ui->btnEnableRight->setEnabled(!enabled);
            ui->btnDisableRight->setEnabled(enabled);
        }
    }
}
```

## 关键点总结

1. **订阅消息格式**：`topics`和`tc`必须在顶层，不在`db`内
2. **推送消息识别**：`ty`字段以`publish/`开头
3. **连接稳定性**：先等500ms，再订阅；订阅后再主动拉取首帧
4. **区分左右臂**：根据`db.name`字段判断
5. **信号定义**：`jointAnglesChanged(QString armName, QVector<double> joints)` - 每次只发送单臂数据

## 测试

1. 连接机器人（输入IP和端口，点击"连接"）
2. 查看终端输出：
   ```
   Subscribing to LeftArm status with tc = 50 ms
   Subscribing to RightArm status with tc = 50 ms
   Received: {"ty":"publish/robotState/sub","topic":"robot/LeftArm/status","db":{...}}
   Robot state updated: ("name", "joints", "cartesian", "enabled", ...)
   Updated left arm joints: [0.1, 0.2, 0.3]
   ```
3. UI应该显示：
   - 关节角度实时更新
   - 使能/失能按钮状态正确
   - 3D仿真同步显示

## 参考

- `/home/gxh/workspace/project/RobotControl/communication/armwebsocket.h`
- `/home/gxh/workspace/project/RobotControl/communication/armwebsocket.cpp`
- API说明文档 §4.8 订阅推送（robotState/sub, projectState/sub）
