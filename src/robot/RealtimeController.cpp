#include "robot/RealtimeController.h"
#include "robot/JointStreamer.h"

#include <QJsonArray>
#include <QThread>
#include <QTimer>
#include <QDebug>
#include <QtMath>

namespace {
constexpr int kJointCount = 7;
constexpr int kLeadInTickMs = 8;

QVector<double> blend(const QVector<double> &from, const QVector<double> &to, double s)
{
    QVector<double> r(kJointCount, 0.0);
    for (int i = 0; i < kJointCount; ++i) {
        const double a = from.value(i, 0.0);
        const double b = to.value(i, 0.0);
        r[i] = a + s * (b - a);
    }
    return r;
}
} // namespace

RealtimeController::RealtimeController(RobotConnection *connection, QObject *parent)
    : QObject(parent)
    , m_connection(connection)
    , m_leadInTimer(new QTimer(this))
    , m_startupWatchdog(new QTimer(this))
{
    // ---- 发送线程 ----
    // 关节流必须脱离 UI 线程：控制器缺帧保持窗只有 5 个锚点周期（下限 30ms），
    // UI 一旦被 3D 重绘或 WebSocket 解析占住就会断顿，被判定为通道陈旧。
    m_streamer = new JointStreamer();
    m_streamThread = new QThread(this);
    m_streamer->moveToThread(m_streamThread);
    connect(m_streamThread, &QThread::finished, m_streamer, &QObject::deleteLater);
    connect(m_streamer, &JointStreamer::statsUpdated,
            this, &RealtimeController::onStreamStats);
    connect(m_streamer, &JointStreamer::sendProblem,
            this, [this](const QString &reason) {
                qWarning() << "RealtimeController: 发送层问题 —" << reason;
                emit statusMessage(QStringLiteral("下发异常：%1").arg(reason));
            });
    m_streamThread->start();

    m_leadInTimer->setTimerType(Qt::PreciseTimer);
    m_leadInTimer->setInterval(kLeadInTickMs);
    connect(m_leadInTimer, &QTimer::timeout, this, &RealtimeController::onLeadInTimer);

    m_startupWatchdog->setSingleShot(true);
    connect(m_startupWatchdog, &QTimer::timeout,
            this, &RealtimeController::checkStartupHealth);

    if (m_connection) {
        connect(m_connection, &RobotConnection::responseReceived,
                this, &RealtimeController::onResponse);
        connect(m_connection, &RobotConnection::disconnected, this, [this]() {
            if (m_streaming || m_busy) {
                qWarning() << "RealtimeController: 连接断开，停止实时控制";
                QMetaObject::invokeMethod(m_streamer, "stop", Qt::QueuedConnection);
                m_leadInTimer->stop();
                m_startupWatchdog->stop();
                m_streaming = false;
                m_busy = false;
                m_leadInActive = false;
                m_playbackActive = false;
                emit streamingStopped();
            }
        });
    }

    m_clock.start();
}

RealtimeController::~RealtimeController()
{
    m_leadInTimer->stop();
    m_startupWatchdog->stop();

    // 退出前必须交还通道，否则会话会残留在控制器上占住该臂，
    // 下次入控会撞上"startControl 无响应 / 包被旧会话吞掉"。
    if (m_streaming && m_connection && m_connection->isConnected()) {
        QJsonObject db;
        if (!m_leftArmName.isEmpty() && !m_rightArmName.isEmpty()) {
            QJsonArray arr;
            arr.append(m_leftArmName);
            arr.append(m_rightArmName);
            db["robot"] = arr;
        } else if (!m_leftArmName.isEmpty()) {
            db["robot"] = m_leftArmName;
        } else if (!m_rightArmName.isEmpty()) {
            db["robot"] = m_rightArmName;
        }
        m_connection->sendCommand(QStringLiteral("realtime/stopControl"), db);
    }

    if (m_streamThread) {
        QMetaObject::invokeMethod(m_streamer, "stop", Qt::QueuedConnection);
        m_streamThread->quit();
        if (!m_streamThread->wait(2000)) {
            qWarning() << "RealtimeController: 发送线程未能及时退出";
            m_streamThread->terminate();
            m_streamThread->wait(500);
        }
    }
}

// ---------------------------------------------------------------- 配置

void RealtimeController::setFrequency(int hz)
{
    m_frequency = qBound(10, hz, 1000);
    if (m_streamer) {
        const int f = m_frequency;
        QMetaObject::invokeMethod(m_streamer, [s = m_streamer, f]() {
            s->setFrequency(f);
        }, Qt::QueuedConnection);
    }
}

void RealtimeController::setCommandPort(quint16 port)
{
    m_commandPort = port;
    if (m_streamer) {
        const quint16 p = port;
        QMetaObject::invokeMethod(m_streamer, [s = m_streamer, p]() {
            s->setCommandPort(p);
        }, Qt::QueuedConnection);
    }
}

void RealtimeController::setHostAddress(const QString &host)
{
    if (host.isEmpty()) {
        return;
    }
    m_host = host;
    m_hostExplicit = true;
    pushHostToStreamer();
}

void RealtimeController::pushHostToStreamer()
{
    if (!m_streamer) {
        return;
    }
    const QString h = m_host;
    QMetaObject::invokeMethod(m_streamer, [s = m_streamer, h]() {
        s->setHost(h);
    }, Qt::QueuedConnection);
}

QString RealtimeController::effectiveHost() const
{
    // 没显式指定时跟随 WebSocket 的连接主机。
    // UDP 指令面和 WebSocket 在同一台控制器上，写死 127.0.0.1 在跨机部署时
    // 会把包发到本机（没人监听），控制器那边表现为目标流始终收不到。
    if (m_hostExplicit) {
        return m_host;
    }
    if (m_connection && !m_connection->host().isEmpty()) {
        return m_connection->host();
    }
    return m_host;
}

void RealtimeController::setArmNames(const QString &left, const QString &right)
{
    if (m_leftArmName == left && m_rightArmName == right) {
        return;
    }
    m_leftArmName = left;
    m_rightArmName = right;
    m_left.name = left;
    m_right.name = right;

    // 发出去的旧臂名不再有用，清掉
    if (m_streamer) {
        const QString l = left;
        const QString r = right;
        QMetaObject::invokeMethod(m_streamer, [s = m_streamer, l, r]() {
            if (l.isEmpty()) {
                s->removeArm(QStringLiteral("left"));
            }
            if (r.isEmpty()) {
                s->removeArm(QStringLiteral("right"));
            }
        }, Qt::QueuedConnection);
    }

    qDebug() << "RealtimeController: 左臂 =" << left << ", 右臂 =" << right;
}

void RealtimeController::setActualJoints(const QString &armName, const QVector<double> &joints)
{
    if (armName.isEmpty() || joints.size() < kJointCount) {
        return;
    }

    // armName 可能是逻辑名（"left"/"right"），也可能是控制器里的真实机器人名。
    // 控制面板发出的 jointAnglesChanged 用的是逻辑名，这里两种都要认。
    const QString key = armName.toLower();

    bool isLeft = false;
    bool isRight = false;

    if (key == QLatin1String("left")) {
        isLeft = true;
    } else if (key == QLatin1String("right")) {
        isRight = true;
    } else {
        if (!m_leftArmName.isEmpty() && armName == m_leftArmName) {
            isLeft = true;
        } else if (!m_rightArmName.isEmpty() && armName == m_rightArmName) {
            isRight = true;
        }
    }

    if (!isLeft && !isRight) {
        qWarning() << "RealtimeController: 无法判定实测关节角属于哪个臂，收到名称 ="
                   << armName;
        return;
    }

    if (isLeft) {
        m_leftActual = joints;
        // 起流前锚点要持续跟着实测走：首包基准越贴近当前姿态越安全
        if (m_streaming && !m_playbackActive && !m_leadInActive) {
            m_left.anchor = joints;
            m_left.latest = joints;
            updateStreamer(m_left);
        }
    } else {
        m_rightActual = joints;
        if (m_streaming && !m_playbackActive && !m_leadInActive) {
            m_right.anchor = joints;
            m_right.latest = joints;
            updateStreamer(m_right);
        }
    }
}

void RealtimeController::resetSessions()
{
    m_left = ArmSession();
    m_right = ArmSession();
    m_left.name = m_leftArmName;
    m_right.name = m_rightArmName;
}

bool RealtimeController::missingActualJoints() const
{
    if (!m_leftArmName.isEmpty() && m_leftActual.size() < kJointCount) {
        return true;
    }
    if (!m_rightArmName.isEmpty() && m_rightActual.size() < kJointCount) {
        return true;
    }
    return false;
}

void RealtimeController::requestActualJoints()
{
    if (!m_connection || !m_connection->isConnected()) {
        return;
    }
    if (!m_leftArmName.isEmpty()) {
        m_connection->getRobotStatus(m_leftArmName);
    }
    if (!m_rightArmName.isEmpty()) {
        m_connection->getRobotStatus(m_rightArmName);
    }
}

// ---------------------------------------------------------------- 入控

void RealtimeController::startRealtime()
{
    if (m_busy || m_streaming) {
        emit errorOccurred(QStringLiteral("实时控制已在进行中"));
        return;
    }
    if (!m_connection || !m_connection->isConnected()) {
        emit errorOccurred(QStringLiteral("未连接到机器人，无法播放"));
        return;
    }
    if (m_leftArmName.isEmpty() && m_rightArmName.isEmpty()) {
        emit errorOccurred(QStringLiteral("尚未识别到机器人，请先连接并确认左右臂"));
        return;
    }

    m_lastError.clear();

    // ---- 前置检查：清掉控制器上遗留的 realtime 会话 ----
    // 控制器上残留的会话（尤其 state="stopped" 的）会占住该臂：
    // startControl 或失败、或包被旧会话吞掉，表现为"发了指令但机械臂不动"，
    // 而且不报任何错。反复入控失败的程序会把这种情况越攒越多，
    // 所以每次入控前必须先清一遍。这段来自已验证的参考实现。
    if (m_connection && !m_cleanupDone) {
        m_cleanupDone = true;     // 只清一次，响应回来再继续
        m_diagRequestId = m_connection->sendCommand(
            QStringLiteral("realtime/getStatus"),
            QJsonObject{{QStringLiteral("robot"), QJsonValue(QJsonValue::Null)}});
        if (m_diagRequestId >= 0) {
            emit statusMessage(QStringLiteral("正在清理遗留的实时会话…"));
            return;
        }
        m_cleanupDone = false;
    }

    // 实测角缺失时先主动拉一帧状态；状态帧是异步回来的
    if (missingActualJoints()) {
        requestActualJoints();
        emit statusMessage(QStringLiteral("正在获取实测关节角…"));

        if (!m_waitingForJoints) {
            m_waitingForJoints = true;
            QTimer::singleShot(500, this, [this]() {
                m_waitingForJoints = false;
                if (missingActualJoints()) {
                    const QString which = (!m_leftArmName.isEmpty()
                                               && m_leftActual.size() < kJointCount)
                                              ? QStringLiteral("左臂")
                                              : QStringLiteral("右臂");
                    emit errorOccurred(
                        QStringLiteral("%1缺少实测关节角，无法安全入控。\n"
                                       "请确认机器人已连接并正常推送状态（使能后重试）。")
                            .arg(which));
                    return;
                }
                startRealtime();
            });
        }
        return;
    }

    resetSessions();

    // 首包基准必须来自实测角
    if (!m_leftArmName.isEmpty()) {
        m_left.anchor = m_leftActual;
        m_left.latest = m_leftActual;
    }
    if (!m_rightArmName.isEmpty()) {
        m_right.anchor = m_rightActual;
        m_right.latest = m_rightActual;
    }

    QJsonArray robots;
    if (!m_leftArmName.isEmpty())  robots.append(m_leftArmName);
    if (!m_rightArmName.isEmpty()) robots.append(m_rightArmName);

    QJsonObject config;
    config["robot"] = robots;
    config["cartesianIkPath"] = QStringLiteral("none");   // 关节流
    config["timeMode"] = QStringLiteral("none");
    config["expectedFrequency"] = m_frequency;
    config["unitType"] = QStringLiteral("si");
    config["interpolator"] = QStringLiteral("linear");
    QJsonObject postFilter;
    postFilter["type"] = QStringLiteral("lowPass");
    postFilter["cutoffFrequency"] = 2;
    config["postFilter"] = postFilter;
    // 停发保持会话：中途卡顿不掉控制权，收尾由 stopControl 显式交还。
    // 这也是 JointStreamer 能在静止后停止下发的依据——停发不会掉线，
    // 会话保持 active、通道仍在直达侧，目标变化时再发包即继续跟随。
    config["holdOnIdle"] = true;

    // 关节限幅是本会话唯一的关节级保护，必须显式打开。
    // 100Hz 下发下 1.5 rad/s / 5.0 rad/s² 同样够用：
    // 发包周期 10ms，单包最大位移 0.015 rad，远小于限幅允许量。
    QJsonObject limit;
    limit["enable"] = true;
    limit["maxVelocity"] = 1.5;       // rad/s
    limit["maxAcceleration"] = 5.0;   // rad/s^2
    config["jointLimit"] = limit;

    QJsonObject db;
    db["config"] = config;

    // ---- 起流：必须在发 startControl 之前 ----
    // 协议要求并行发首包。发送线程先跑起来，把实测锚点推过去，
    // 这样 startControl 的响应还没回来时，首包已经在路上了。
    m_clock.restart();

    const QString leftKey  = m_leftArmName;
    const QString rightKey = m_rightArmName;
    const QVector<double> leftAnchor  = m_left.anchor;
    const QVector<double> rightAnchor = m_right.anchor;
    const QString host = effectiveHost();
    const quint16 port = m_commandPort;
    const int freq = m_frequency;

    qDebug() << "RealtimeController: UDP 目标" << host << ":" << port
             << "频率" << freq << "Hz";

    // 用 BlockingQueuedConnection 强制"发送线程已开始发包"后再往下走。
    // 普通 QueuedConnection 只是把任务塞进发送线程的事件队列，
    // 本线程会立刻继续执行下一行，无法保证发包先于 startControl 生效；
    // 而首包等待窗只有 2000ms，起流晚一步就可能被判欠载、回收通道。
    QMetaObject::invokeMethod(m_streamer, [s = m_streamer, leftKey, rightKey,
                                           leftAnchor, rightAnchor,
                                           host, port, freq]() {
        s->setHost(host);
        s->setCommandPort(port);
        s->setFrequency(freq);
        // 静止 1 秒后暂停下发。配合 holdOnIdle:true，会话不会因此掉线，
        // 目标一变就自动恢复——停在原地时伺服上那点持续的激励声也就没了。
        s->setIdleStopMs(1000);
        if (!leftKey.isEmpty())  s->setTarget(leftKey, leftAnchor);
        if (!rightKey.isEmpty()) s->setTarget(rightKey, rightAnchor);
        s->start();
    }, Qt::BlockingQueuedConnection);

    m_streaming = true;     // 逻辑上已起流，锚点会持续刷新
    m_busy = true;

    m_startRequestId = m_connection->sendCommand(QStringLiteral("realtime/startControl"), db);

    if (m_startRequestId < 0) {
        m_busy = false;
        m_streaming = false;
        QMetaObject::invokeMethod(m_streamer, "stop", Qt::QueuedConnection);
        emit errorOccurred(QStringLiteral("发送 realtime/startControl 失败"));
        return;
    }

    m_statusProbed = false;   // 本次已查完；下次入控重新确认

    emit statusMessage(QStringLiteral("正在进入实时控制模式（%1Hz 下发）…")
                           .arg(m_frequency));

    // 入控响应迟迟不来时给出可诊断的提示，而不是干等到超时
    m_startupWatchdog->start(4000);
}

void RealtimeController::checkStartupHealth()
{
    if (m_busy && m_startRequestId >= 0) {
        qWarning() << "RealtimeController: startControl 4 秒未响应，抓取会话诊断";
        emit statusMessage(
            QStringLiteral("入控响应较慢，正在检查指令流是否已被控制器接收…"));
        // 关键诊断：receivedCount 是控制器"已接收的合法报文数"。
        //   = 0  → 包没到控制器（网络/地址/端口问题）
        //   > 0  → 包到了但没能推进启动（数据内容或时序问题）
        // 两种情况的处置完全不同，必须区分开。
        probeSessionDiagnostics();
        m_startupWatchdog->start(4000);
    }
}

void RealtimeController::probeSessionDiagnostics()
{
    if (!m_connection || !m_connection->isConnected()) {
        return;
    }
    m_probeRequestId = m_connection->sendCommand(
        QStringLiteral("realtime/getStatus"),
        QJsonObject{{QStringLiteral("robot"), QJsonValue(QJsonValue::Null)}});
}

void RealtimeController::reportSessionDiagnostics(const QJsonObject &db)
{
    const QJsonArray sessions = db.value(QStringLiteral("sessions")).toArray();
    if (sessions.isEmpty()) {
        qWarning() << "RealtimeController: 控制器没有任何实时会话"
                   << "（startControl 可能尚未被受理）";
        emit statusMessage(QStringLiteral(
            "控制器侧没有本次会话记录，入控请求可能未被受理。"));
        return;
    }

    for (const QJsonValue &v : sessions) {
        const QJsonObject s = v.toObject();
        const QString robot = s.value(QStringLiteral("robot")).toString();
        const QString state = s.value(QStringLiteral("state")).toString();
        const QString err   = s.value(QStringLiteral("error")).toString();
        const int received  = s.value(QStringLiteral("receivedCount")).toInt(-1);
        const int published = s.value(QStringLiteral("publishedCount")).toInt(-1);
        const int dropped   = s.value(QStringLiteral("droppedCount")).toInt(-1);
        const int missing   = s.value(QStringLiteral("missingAnchorCount")).toInt(-1);

        qWarning().nospace()
            << "  会话 " << robot << ": state=" << state
            << " error=" << (err.isEmpty() ? QStringLiteral("(无)") : err)
            << " receivedCount=" << received
            << " publishedCount=" << published
            << " droppedCount=" << dropped
            << " missingAnchorCount=" << missing;

        if (received == 0) {
            emit statusMessage(QStringLiteral(
                "%1：控制器一个合法报文都没收到（receivedCount=0）。"
                "说明 UDP 包没到达控制器——请检查控制器地址、9030 端口与防火墙（UDP）。")
                    .arg(robot));
        } else {
            emit statusMessage(QStringLiteral(
                "%1：控制器已收到 %2 个报文，但启动未推进（state=%3）。"
                "包已到达，问题在报文内容或首包时序。")
                    .arg(robot).arg(received).arg(state));
        }
    }
}

void RealtimeController::onResponse(const QJsonObject &response)
{
    const QString ty = response.value("ty").toString();
    const int id = response.value("id").toInt(-1);

    if (ty == QLatin1String("realtime/getStatus") && id == m_probeRequestId) {
        m_probeRequestId = -1;
        reportSessionDiagnostics(response.value(QStringLiteral("db")).toObject());
        return;
    }

    if (ty == QLatin1String("realtime/getStatus") && id == m_diagRequestId) {
        m_diagRequestId = -1;

        if (response.contains(QStringLiteral("err"))) {
            const QJsonObject err = response.value("err").toObject();
            qWarning() << "RealtimeController: getStatus 失败"
                       << err.value("code").toInt()
                       << err.value("message").toString();
        } else {
            const QJsonObject db = response.value(QStringLiteral("db")).toObject();

            // 端口是配置项（缺省 9030），以控制器回读为准
            const int port = db.value(QStringLiteral("commandPort")).toInt(0);
            if (port > 0 && port <= 65535) {
                setCommandPort(quint16(port));
            }

            if (db.contains(QStringLiteral("commandPortReady"))
                && !db.value(QStringLiteral("commandPortReady")).toBool(true)) {
                m_lastError = QStringLiteral(
                    "控制器实时指令端口未就绪（commandPortReady=false），无法入控。");
                qWarning() << "RealtimeController:" << m_lastError;
                m_cleanupDone = false;
                emit errorOccurred(m_lastError);
                return;
            }

            // 残留会话会占住该臂，必须停掉再入控
            const QJsonArray sessions = db.value(QStringLiteral("sessions")).toArray();
            QStringList stale;
            for (const QJsonValue &v : sessions) {
                const QString robot = v.toObject().value(QStringLiteral("robot")).toString();
                if (robot.isEmpty()) {
                    continue;
                }
                if ((!m_leftArmName.isEmpty() && robot == m_leftArmName)
                    || (!m_rightArmName.isEmpty() && robot == m_rightArmName)) {
                    stale << robot;
                }
            }

            if (!stale.isEmpty() && m_cleanupRetries < 3) {
                ++m_cleanupRetries;
                for (const QString &robot : stale) {
                    qWarning() << "RealtimeController: 清理遗留会话" << robot;
                    QJsonObject sdb{{QStringLiteral("robot"), robot}};
                    m_connection->sendCommand(QStringLiteral("realtime/stopControl"), sdb);
                }
                emit statusMessage(QStringLiteral("已清理遗留会话：%1，重新入控…")
                                       .arg(stale.join(QStringLiteral(", "))));
                // 给控制器一点时间完成清理，再重走入控
                m_cleanupDone = false;
                QTimer::singleShot(300, this, [this]() {
                    startRealtime();
                });
                return;
            }

            if (!stale.isEmpty()) {
                qWarning() << "RealtimeController: 清理 3 次后仍存在遗留会话" << stale
                           << "，继续尝试入控";
            }
        }

        startRealtime();
        return;
    }

    if (ty == QLatin1String("realtime/getStatus") && id == m_statusRequestId) {
        m_statusRequestId = -1;

        if (response.contains(QStringLiteral("err"))) {
            const QJsonObject err = response.value("err").toObject();
            m_lastError = QStringLiteral("real-time 状态查询失败 (%1): %2")
                              .arg(err.value("code").toInt())
                              .arg(err.value("message").toString());
            qWarning() << "RealtimeController:" << m_lastError;
            emit errorOccurred(m_lastError);
            return;
        }

        const QJsonObject db = response.value("db").toObject();

        // 端口是配置项，以控制器回读为准
        const int port = db.value("commandPort").toInt(0);
        if (port > 0 && port <= 65535) {
            setCommandPort(quint16(port));
        }

        if (db.contains(QStringLiteral("commandPortReady"))
            && !db.value(QStringLiteral("commandPortReady")).toBool(true)) {
            m_lastError = QStringLiteral(
                "控制器实时指令端口未就绪（commandPortReady=false），无法入控。\n"
                "请检查控制器的 realtime 配置与端口占用。");
            qWarning() << "RealtimeController:" << m_lastError;
            emit errorOccurred(m_lastError);
            return;
        }

        qDebug() << "RealtimeController: 指令端口就绪，端口 =" << m_commandPort
                 << ", 目标主机 =" << effectiveHost();

        // 继续走完整入控流程
        startRealtime();
        return;
    }

    if (ty == QLatin1String("realtime/startControl") && id == m_startRequestId) {
        m_startRequestId = -1;
        m_startupWatchdog->stop();

        if (response.contains(QStringLiteral("err"))) {
            const QJsonObject err = response.value("err").toObject();
            m_lastError = QStringLiteral("入控失败 (%1): %2")
                              .arg(err.value("code").toInt())
                              .arg(err.value("message").toString());
            qWarning() << "RealtimeController:" << m_lastError;

            QMetaObject::invokeMethod(m_streamer, "stop", Qt::QueuedConnection);
            m_streaming = false;
            m_busy = false;
            emit errorOccurred(m_lastError);
            return;
        }

        m_left.active  = !m_leftArmName.isEmpty();
        m_right.active = !m_rightArmName.isEmpty();
        m_busy = false;

        if (m_left.active)  emit armSessionChanged(m_leftArmName, true);
        if (m_right.active) emit armSessionChanged(m_rightArmName, true);

        emit streamingStarted();
        emit statusMessage(QStringLiteral("已进入实时控制，正在对齐起点…"));
        return;
    }

    if (ty == QLatin1String("realtime/stopControl") && id == m_stopRequestId) {
        m_stopRequestId = -1;
        m_busy = false;

        const bool leftWasActive = m_left.active;
        const bool rightWasActive = m_right.active;
        m_left.active = false;
        m_right.active = false;

        if (leftWasActive)  emit armSessionChanged(m_leftArmName, false);
        if (rightWasActive) emit armSessionChanged(m_rightArmName, false);

        emit streamingStopped();
        emit statusMessage(QStringLiteral("已退出实时控制"));
    }
}

void RealtimeController::onStreamStats(int actualHz, qint64 totalPackets)
{
    emit streamStats(actualHz, totalPackets);

    // 频率明显不达标说明系统有卡顿，值得提示——这直接关系到会不会被判通道陈旧
    if (m_frequency >= 50 && actualHz > 0 && actualHz < m_frequency * 0.7) {
        qWarning() << "RealtimeController: 实际下发" << actualHz
                   << "Hz，低于设定" << m_frequency << "Hz";
    }
}

// ---------------------------------------------------------------- 发帧

void RealtimeController::updateStreamer(ArmSession &arm)
{
    if (!m_streamer || arm.name.isEmpty() || arm.latest.size() < kJointCount) {
        return;
    }
    const QString name = arm.name;
    const QVector<double> v = arm.latest;
    QMetaObject::invokeMethod(m_streamer, [s = m_streamer, name, v]() {
        s->setTarget(name, v);
    }, Qt::QueuedConnection);
}

void RealtimeController::pushFrame(const JointState &state)
{
    if (!m_streaming) {
        return;
    }

    QVector<double> left(kJointCount), right(kJointCount);
    for (int i = 0; i < kJointCount; ++i) {
        left[i] = state.j[i];
        right[i] = state.jr[i];
    }

    if (!m_playbackActive) {
        if (m_leadInActive) {
            return;     // 过渡中由 onLeadInTimer 推进
        }
        // 收到轨迹起点：从实测角平滑过渡过去。
        // 这是绕开 firstFrameJumpRejected 的关键——首包必须贴近实测。
        if (m_left.active) {
            m_left.leadFrom = m_left.anchor;
            m_left.leadTo = left;
        }
        if (m_right.active) {
            m_right.leadFrom = m_right.anchor;
            m_right.leadTo = right;
        }
        m_leadInStartMs = m_clock.elapsed();
        m_leadInActive = true;
        m_leadInTimer->start();
        emit statusMessage(QStringLiteral("正在平滑对齐轨迹起点…"));
        return;
    }

    // 正常播放：更新目标即可，发送线程按自己的频率发最新值
    if (m_left.active) {
        m_left.latest = left;
        updateStreamer(m_left);
    }
    if (m_right.active) {
        m_right.latest = right;
        updateStreamer(m_right);
    }
}

void RealtimeController::onLeadInTimer()
{
    if (!m_leadInActive || !m_streaming) {
        m_leadInTimer->stop();
        m_leadInActive = false;
        return;
    }

    double u = (m_clock.elapsed() - m_leadInStartMs) / qMax(1.0, m_leadInDurationMs);
    u = qBound(0.0, u, 1.0);
    const double s = u * u * (3.0 - 2.0 * u);   // S 曲线，起止速度为 0

    if (m_left.active) {
        m_left.latest = blend(m_left.leadFrom, m_left.leadTo, s);
        updateStreamer(m_left);
    }
    if (m_right.active) {
        m_right.latest = blend(m_right.leadFrom, m_right.leadTo, s);
        updateStreamer(m_right);
    }

    if (u >= 1.0) {
        m_leadInTimer->stop();
        m_leadInActive = false;
        m_playbackActive = true;
        emit statusMessage(QStringLiteral("已对齐起点，开始播放"));
        // 通知时间轴此刻才推进：过渡期的帧是被丢弃的，
        // 若时间轴提前跑，机器人会在这里一步跳到 700ms 后的位置
        emit leadInFinished();
    }
}

bool RealtimeController::sendJoints(ArmSession &arm, const QVector<double> &joints)
{
    // 发送已移交 JointStreamer，这里只为兼容保留
    if (arm.name.isEmpty() || joints.size() < kJointCount) {
        return false;
    }
    arm.latest = joints;
    updateStreamer(arm);
    return true;
}

// ---------------------------------------------------------------- 停控

void RealtimeController::stopRealtime()
{
    const bool wasStreaming = m_streaming;
    const bool wasBusy = m_busy;

    QMetaObject::invokeMethod(m_streamer, "stop", Qt::QueuedConnection);

    m_leadInTimer->stop();
    m_startupWatchdog->stop();
    m_leadInActive = false;
    m_playbackActive = false;
    m_streaming = false;

    // 复位入控前置检查标志：下次播放必须重新清残留会话、重新确认端口。
    // 不复位会让第二次播放跳过清理，直接撞上残留会话。
    m_cleanupDone = false;
    m_cleanupRetries = 0;
    m_statusProbed = false;
    if (!wasStreaming && !wasBusy) {
        return;
    }

    if (m_connection && m_connection->isConnected()) {
        sendStopControl();
    } else {
        m_busy = false;
        emit streamingStopped();
    }
}

void RealtimeController::sendStopControl()
{
    QJsonObject db;
    if (!m_leftArmName.isEmpty() && !m_rightArmName.isEmpty()) {
        QJsonArray arr;
        arr.append(m_leftArmName);
        arr.append(m_rightArmName);
        db["robot"] = arr;
    } else if (!m_leftArmName.isEmpty()) {
        db["robot"] = m_leftArmName;
    } else if (!m_rightArmName.isEmpty()) {
        db["robot"] = m_rightArmName;
    } else {
        db["robot"] = QJsonValue(QJsonValue::Null);
    }

    m_busy = true;
    m_stopRequestId = m_connection->sendCommand(
        QStringLiteral("realtime/stopControl"), db);

    if (m_stopRequestId < 0) {
        m_busy = false;
        m_left.active = false;
        m_right.active = false;
        emit streamingStopped();
    }
}
