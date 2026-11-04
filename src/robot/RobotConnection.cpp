#include "robot/RobotConnection.h"
#include <QJsonArray>
#include <QDebug>
#include <QUrl>

RobotConnection::RobotConnection(QObject *parent)
    : QObject(parent)
    , m_webSocket(new QWebSocket())
    , m_messageId(1)
    , m_isConnected(false)
{
    m_webSocket->setParent(this);
    
    connect(m_webSocket, &QWebSocket::connected, 
            this, &RobotConnection::onConnected);
    connect(m_webSocket, &QWebSocket::disconnected, 
            this, &RobotConnection::onDisconnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived, 
            this, &RobotConnection::onTextMessageReceived);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_webSocket, &QWebSocket::errorOccurred,
            this, &RobotConnection::onError);
#else
    connect(m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, &RobotConnection::onError);
#endif
}

RobotConnection::~RobotConnection()
{
    if (m_webSocket) {
        m_webSocket->close();
        delete m_webSocket;
    }
}

bool RobotConnection::connectToRobot(const QString& host, quint16 port)
{
    m_host = host;      // 记住主机：realtime 的 UDP 指令面要发到同一台控制器
    QString url = QString("ws://%1:%2").arg(host).arg(port);
    qDebug() << "==========================================";
    qDebug() << "Connecting to WebSocket:" << url;
    qDebug() << "Host:" << host << "Port:" << port;
    
    // 检查是否已连接
    if (m_webSocket->state() == QAbstractSocket::ConnectedState) {
        qDebug() << "Already connected, disconnecting first...";
        m_webSocket->close();
        QTimer::singleShot(500, this, [this, url]() {
            qDebug() << "Retrying connection to:" << url;
            m_webSocket->open(QUrl(url));
        });
        return true;
    }
    
    qDebug() << "Opening WebSocket connection...";
    m_webSocket->open(QUrl(url));
    qDebug() << "Connection initiated, state:" << m_webSocket->state();
    qDebug() << "==========================================";
    return true;
}

void RobotConnection::disconnect()
{
    m_webSocket->close();
}

bool RobotConnection::isConnected() const
{
    return m_isConnected;
}

int RobotConnection::sendCommand(const QString& ty, const QJsonObject& db, int id)
{
    if (!m_isConnected) {
        emit error("Not connected to robot");
        return -1;
    }
    
    if (id == -1) {
        id = getNextId();
    }
    
    QJsonObject message;
    message["id"] = id;
    message["ty"] = ty;
    message["db"] = db;
    
    // 记录在途请求，响应回来时用它补回机器人名
    PendingRequest pending;
    pending.action = ty;
    pending.robotName = db.value("name").toString();
    m_pendingRequests[id] = pending;
    
    QJsonDocument doc(message);
    QString jsonString = doc.toJson(QJsonDocument::Compact);
    
    qDebug() << "Sending:" << jsonString;
    m_webSocket->sendTextMessage(jsonString);
    return id;
}

void RobotConnection::subscribeToState(const QString& robotName, int tc)
{
    if (!m_isConnected) {
        qWarning() << "Cannot subscribe: not connected";
        return;
    }
    
    qDebug() << "Subscribing to" << robotName << "status with tc =" << tc << "ms";
    
    // 订阅消息格式（订阅字段放顶层，不在db内！）
    QJsonObject sub;
    sub["id"] = getNextId();
    sub["ty"] = "robotState/sub";
    
    // topics是字符串（参考armwebsocket.cpp:676）
    sub["topics"] = QString("robot/%1/status").arg(robotName);
    sub["tc"] = tc;
    
    QJsonDocument doc(sub);
    QString jsonStr = doc.toJson(QJsonDocument::Compact);
    qDebug() << "Sending subscription:" << jsonStr;
    
    m_webSocket->sendTextMessage(jsonStr);
}

void RobotConnection::listRobots()
{
    sendCommand("robot/list", QJsonObject());
}

void RobotConnection::enableRobot(const QString& name)
{
    QJsonObject db;
    db["name"] = name;
    sendCommand("robot/enable", db);
}

void RobotConnection::disableRobot(const QString& name)
{
    QJsonObject db;
    db["name"] = name;
    sendCommand("robot/disable", db);
}

void RobotConnection::getRobotStatus(const QString& name)
{
    QJsonObject db;
    db["name"] = name;
    sendCommand("robot/status", db);
}

int RobotConnection::jogStart(const QString& name, const QString& space,
                               const QString& axis, int direction,
                               double speed, double maxTarget, int timeoutMs)
{
    // 连续点动：报文字段与 jogStep 不同，别照抄。
    // jogStart 要求的是一组「持续运动」参数，缺一个必填项就是 20000 参数无效：
    //   speed      点动速度上限（rad/s、m/s）——必填，0 表示取机型安全缺省档
    //   maxTarget  该方向最大行程——必填，0 表示自动取可达最远点
    //   timeoutMs  心跳超时——必填，<=0 取缺省 500ms
    //   slaves     从臂列表——键必填，单臂点动写空数组
    // 少发这些字段时控制器直接同步拒绝，界面上表现为「长按没反应」。
    QJsonObject db;
    db["name"] = name;
    db["space"] = space;
    db["axis"] = axis;
    db["direction"] = direction;
    db["speed"] = speed;
    db["maxTarget"] = maxTarget;
    db["timeoutMs"] = timeoutMs;
    db["slaves"] = QJsonArray();    // 单臂点动：键必须存在，值为空数组

    return sendCommand("robot/jogStart", db);
}

void RobotConnection::jogHeartbeat(const QString& name)
{
    QJsonObject db;
    db["name"] = name;
    sendCommand("robot/jogHeartbeat", db);
}

void RobotConnection::jogStop(const QString& name)
{
    QJsonObject db;
    db["name"] = name;
    sendCommand("robot/jogStop", db);
}

void RobotConnection::onConnected()
{
    m_isConnected = true;
    qDebug() << "==========================================";
    qDebug() << "✓ WebSocket connected successfully!";
    qDebug() << "✓ State:" << m_webSocket->state();
    qDebug() << "==========================================";
    emit connected();
    
    // 连接后立即拉一次机器人清单，订阅动作由上层在拿到真实臂名后发起
    // （不再在这里硬编码 "LeftArm"/"RightArm"，那些名字通常并不存在）
    QTimer::singleShot(200, this, [this]() {
        listRobots();
    });
}

void RobotConnection::onDisconnected()
{
    m_isConnected = false;
    qWarning() << "==========================================";
    qWarning() << "✗ WebSocket disconnected";
    qWarning() << "✗ Close code:" << m_webSocket->closeCode();
    qWarning() << "✗ Close reason:" << m_webSocket->closeReason();
    qWarning() << "==========================================";
    emit disconnected();
}

void RobotConnection::onTextMessageReceived(const QString& message)
{
    // 可能一次收到多行JSON
    QStringList lines = message.split('\n', Qt::SkipEmptyParts);
    
    for (const QString& line : lines) {
        if (line.trimmed().isEmpty()) continue;
        
        qDebug() << "Received:" << line;
        
        QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
        if (!doc.isObject()) {
            qWarning() << "Invalid JSON received:" << line;
            continue;
        }
        
        QJsonObject obj = doc.object();
        QString ty = obj.value("ty").toString();
        
        qDebug() << "  -> ty:" << ty;
        qDebug() << "  -> Has 'topic'?" << obj.contains("topic");
        qDebug() << "  -> Has 'id'?" << obj.contains("id");
        
        // 检查是否是推送消息（状态更新）
        // 推送消息格式：{ "ty": "publish/robot/LeftArm/status", "db": {...} }
        // 注意：机器人名称在 ty 字段里，不是在单独的 topic 字段！
        if (obj.contains("ty")) {
            
            if (ty.startsWith("publish/")) {
                // 这是订阅推送消息
                QJsonObject db = obj.value("db").toObject();
                
                qDebug() << "  -> Push message, ty:" << ty;
                
                // 从 ty 中提取机器人名称
                // ty格式: "publish/robot/LeftArm/status" 或 "publish/robot/RightArm/status"
                if (ty.contains("/status")) {
                    // 移除 "publish/" 前缀
                    QString topic = ty.mid(8);  // "robot/LeftArm/status"
                    qDebug() << "  -> Extracted topic from ty:" << topic;
                    
                    if (topic.startsWith("robot/") && topic.endsWith("/status")) {
                        QString robotName = topic.mid(6, topic.length() - 6 - 7);  // 提取 "robot/" 和 "/status" 之间的部分
                        qDebug() << "  -> Extracted robot name:" << robotName;
                        
                        // 将机器人名称添加到db中
                        QJsonObject dbWithName = db;
                        dbWithName["name"] = robotName;
                        
                        emit robotStateUpdated(dbWithName);
                    } else {
                        qDebug() << "  -> Topic doesn't match pattern, emitting without name";
                        emit robotStateUpdated(db);
                    }
                }
                continue;
            } else {
                qDebug() << "  -> Not a publish message (doesn't start with 'publish/')";
            }
        }
        
        // 处理响应消息
        if (obj.contains("id")) {
            int id = obj["id"].toInt();
            const PendingRequest pending = m_pendingRequests.value(id);
            QString ty = pending.action;
            
            qDebug() << "  -> Response for id:" << id << "ty:" << ty;
            
            if (ty == "robot/list" && obj.contains("db")) {
                QJsonValue dbValue = obj["db"];
                QJsonArray robots;
                
                // db可能直接是数组，也可能是{items: []}
                if (dbValue.isArray()) {
                    robots = dbValue.toArray();
                } else if (dbValue.isObject()) {
                    robots = dbValue.toObject()["items"].toArray();
                }
                
                qDebug() << "  -> Robot list received:" << robots.size() << "robots";
                emit robotListReceived(robots);
            } else if (ty == "robot/status" && obj.contains("db")) {
                // 单次状态查询的响应：db 里没有 name 字段，必须从在途请求补回，
                // 否则上层无法区分是哪个臂的状态。
                QJsonObject db = obj["db"].toObject();
                QString robotName = db.value("name").toString();
                if (robotName.isEmpty()) {
                    robotName = pending.robotName;
                }
                if (!robotName.isEmpty()) {
                    db["name"] = robotName;
                } else {
                    qWarning() << "  -> robot/status response without robot name, id:" << id;
                }
                emit robotStateUpdated(db);
            }
            
            m_pendingRequests.remove(id);
        }
        
        emit responseReceived(obj);
    }
}

void RobotConnection::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    QString errorString = m_webSocket->errorString();
    qWarning() << "==========================================";
    qWarning() << "✗ WebSocket error:" << errorString;
    qWarning() << "✗ Error code:" << m_webSocket->error();
    qWarning() << "✗ State:" << m_webSocket->state();
    qWarning() << "==========================================";
    emit this->error(errorString);
}

int RobotConnection::getNextId()
{
    return m_messageId++;
}
