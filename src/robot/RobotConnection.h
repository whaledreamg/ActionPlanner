#ifndef ROBOTCONNECTION_H
#define ROBOTCONNECTION_H

#include <QObject>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QMap>
#include <QString>

class RobotConnection : public QObject
{
    Q_OBJECT

public:
    explicit RobotConnection(QObject *parent = nullptr);
    ~RobotConnection();

    bool connectToRobot(const QString& host, quint16 port = 9000);
    void disconnect();
    bool isConnected() const;

    /**
     * 当前连接的主机名/IP。
     * realtime 的 UDP 指令面与 WebSocket 在同一台控制器上，需要用它来定位目标地址。
     */
    QString host() const { return m_host; }

    // Send commands
    /** 发送请求。返回消息 id，可用于匹配响应。 */
    int sendCommand(const QString& ty, const QJsonObject& db, int id = -1);
    void subscribeToState(const QString& robotName, int tc = 50);

    // Robot control
    void listRobots();
    void enableRobot(const QString& name);
    void disableRobot(const QString& name);
    void getRobotStatus(const QString& name);

    /**
     * 连续点动（长按持续走，松开即停）。
     *
     * speed / maxTarget 传 0 让控制器取该机型的安全缺省档与可达最远点——
     * 界面上的点动按钮没有速度设置项，硬编码一个数值反而会与机型的
     * 安全上限打架。timeoutMs 必须大于心跳周期，否则每次心跳前就已经超时停住。
     *
     * 返回请求 id，供上层把本次 jogStart 的拒绝原因精确匹配出来
     * （缺速度、未使能、控制模式不对等都是同步返回 err）。
     */
    int jogStart(const QString& name, const QString& space, const QString& axis,
                 int direction, double speed = 0.0, double maxTarget = 0.0,
                 int timeoutMs = 500);
    void jogHeartbeat(const QString& name);
    void jogStop(const QString& name);

signals:
    void connected();
    void disconnected();
    void error(const QString& message);
    void responseReceived(const QJsonObject& response);

    /** 状态帧。保证 state["name"] 恒为该帧对应的机器人名（推送帧与查询响应都补齐） */
    void robotStateUpdated(const QJsonObject& state);
    void robotListReceived(const QJsonArray& robots);
    void topicPushReceived(const QString& topic, const QJsonObject& db);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString& message);
    void onError(QAbstractSocket::SocketError error);

private:
    int getNextId();

    /** 在途请求上下文：用于给响应帧补回机器人名 */
    struct PendingRequest {
        QString action;    // "robot/status" 等
        QString robotName; // 请求里带的 name（可能为空）
    };

    QWebSocket *m_webSocket;
    int m_messageId;
    QString m_host;              ///< 当前连接的主机，realtime 用它定位 UDP 目标
    QString m_connectedRobotName;
    bool m_isConnected;
    QMap<int, PendingRequest> m_pendingRequests;
};

#endif // ROBOTCONNECTION_H
