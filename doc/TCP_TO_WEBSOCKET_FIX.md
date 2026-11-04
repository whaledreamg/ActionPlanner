# 关键修复：QTcpSocket → QWebSocket

## 核心问题

**你用的是`QTcpSocket`，但机械臂API需要`QWebSocket`协议！**

这就是为什么连接后状态没有更新的根本原因。

## 问题分析

### 参考实现使用WebSocket
```cpp
// /home/gxh/workspace/project/RobotControl/communication/armwebsocket.h
class ArmWebSocket : public QObject
{
    Q_OBJECT
public:
    ...
private:
    QWebSocket *m_socket = nullptr;  // ← WebSocket！
    ...
};
```

### 你之前的错误实现
```cpp
// ActionPlanner/src/robot/RobotConnection.h
#include <QTcpSocket>  // ← 错了！

class RobotConnection : public QObject {
    ...
private:
    QTcpSocket *m_webSocket;  // ← 应该是QWebSocket！
};
```

## 修复内容

### 1. 头文件引入

```cpp
// RobotConnection.h
#include <QWebSocket>  // 改为WebSocket
```

### 2. 构造函数

```cpp
RobotConnection::RobotConnection(QObject *parent)
    : QObject(parent)
    , m_webSocket(new QWebSocket())  // 改为QWebSocket
    , m_messageId(1)
    , m_isConnected(false)
{
    m_webSocket->setParent(this);
    
    // WebSocket的信号
    connect(m_webSocket, &QWebSocket::connected, 
            this, &RobotConnection::onConnected);
    connect(m_webSocket, &QWebSocket::disconnected, 
            this, &RobotConnection::onDisconnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived,  // 改为textMessageReceived
            this, &RobotConnection::onTextMessageReceived);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_webSocket, &QWebSocket::errorOccurred,
            this, &RobotConnection::onError);
#else
    connect(m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, &RobotConnection::onError);
#endif
}
```

### 3. 连接方法

```cpp
bool RobotConnection::connectToRobot(const QString& host, quint16 port)
{
    QString url = QString("ws://%1:%2").arg(host).arg(port);  // WebSocket URL格式
    qDebug() << "Connecting to WebSocket:" << url;
    m_webSocket->open(QUrl(url));  // 改为open()
    return true;
}
```

**关键区别**：
- TCP: `m_socket->connectToHost(host, port);`
- WebSocket: `m_socket->open(QUrl("ws://host:port"));`

### 4. 发送消息

```cpp
void RobotConnection::sendCommand(const QString& ty, const QJsonObject& db, int id)
{
    ...
    QString jsonString = doc.toJson(QJsonDocument::Compact);
    m_webSocket->sendTextMessage(jsonString);  // 改为sendTextMessage()
    // 不需要 write() 和 flush()，不需要添加 \n
}

void RobotConnection::subscribeToState(const QString& robotName, int tc)
{
    ...
    QString jsonStr = doc.toJson(QJsonDocument::Compact);
    m_webSocket->sendTextMessage(jsonStr);  // 改为sendTextMessage()
}
```

**关键区别**：
- TCP: `m_socket->write(data); m_socket->flush();` 需要手动添加`\n`
- WebSocket: `m_socket->sendTextMessage(data);` 自动处理帧边界

### 5. 接收消息

```cpp
// WebSocket自动处理消息边界，每个完整JSON自动触发textMessageReceived信号
void RobotConnection::onTextMessageReceived(const QString& message)
{
    // message已经是完整的一个JSON，但可能包含多行
    QStringList lines = message.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
        ...
    }
}
```

## TCP vs WebSocket 关键区别

| 特性 | TCP (QTcpSocket) | WebSocket (QWebSocket) |
|------|------------------|------------------------|
| 协议 | 原始TCP流 | 基于HTTP的WebSocket协议 |
| 消息边界 | 无，需要手动处理 | 有，自动处理帧 |
| 连接方式 | `connectToHost(host, port)` | `open(QUrl("ws://host:port"))` |
| 发送 | `write(data)` + `flush()` | `sendTextMessage(text)` |
| 接收 | `readyRead()` + `readAll()` | `textMessageReceived(message)` |
| 换行符 | 需要手动添加`\n` | 不需要 |
| 适用场景 | 自定义协议 | Web API、实时推送 |

## 为什么之前状态没更新？

1. **协议不匹配**：机械臂控制器只支持WebSocket，用TCP连不上或连上也无法通信
2. **消息格式错误**：TCP需要`\n`分隔，WebSocket不需要
3. **信号不对**：`readyRead`需要手动读，`textMessageReceived`自动传递完整消息

## 测试

现在运行程序：

```bash
cd /home/gxh/workspace/project/ActionPlanner
./ActionPlanner
```

连接机器人后，终端应该看到：

```
Connecting to WebSocket: ws://192.168.1.100:9000
WebSocket connected successfully
Starting subscriptions...
Subscribing to robot states...
Subscribing to LeftArm status with tc = 50 ms
Sending subscription: {"id":2,"tc":50,"topics":["robot/LeftArm/status"],"ty":"robotState/sub"}
Fetching initial status...
Received: {"ty":"publish/robotState/sub","topic":"robot/LeftArm/status","db":{...}}
=== onRobotStateUpdated called ===
State keys: ("name", "joints", "cartesian", "enabled", ...)
Robot name: LeftArm
Joints count: 7
Is left arm: true
Updated LEFT arm, emitted signal
First 3 joints: [0.1, 0.2, 0.3]
=== onRobotStateUpdated finished ===
```

UI应该实时更新关节角度了！✅
