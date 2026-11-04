#ifndef REALTIME_CONTROLLER_H
#define REALTIME_CONTROLLER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QElapsedTimer>

#include "robot/RobotConnection.h"
#include "timeline/TimelineModel.h"

class JointStreamer;
class QThread;
class QTimer;

/**
 * 双臂 realtime 实时控制。
 *
 * 控制面：WebSocket（复用 RobotConnection）发 realtime/startControl / stopControl
 * 数据面：UDP 9030，扁平 JSON，一包一个关节目标，不回包。由 JointStreamer
 *         在独立线程按固定频率发送，避免 UI 卡顿导致断流被回收通道。
 *
 * 协议约束（踩错就是机器人不动或直接被停控）：
 *  1. 发起 startControl 后必须立刻并行发首包，不能等响应。首次入控构建求解器
 *     可能耗时数百毫秒，等着发就会错过 2000ms 首包等待窗，控制器随即报
 *     「直达通道陈旧，已自动回收到规划通道」。
 *  2. 首包必须贴近入控瞬间的实测关节角，否则 firstFrameJumpRejected 并停控。
 *     所以先持续发实测锚点，收到轨迹起点后再平滑过渡。
 *  3. 缺帧保持窗只有 5 个锚点周期（下限 30ms），发送必须稳定不抖。
 *  4. 只有 stopControl 才交还通道；停发不会自动掉线。
 */
class RealtimeController : public QObject
{
    Q_OBJECT

public:
    explicit RealtimeController(RobotConnection *connection, QObject *parent = nullptr);
    ~RealtimeController();

    bool isStreaming() const { return m_streaming; }

    /**
     * 起点对齐是否已完成。
     *
     * isStreaming() 为真只代表通道已建立，此时机器人可能还在从实测角
     * 平滑靠向轨迹起点，过渡期收到的帧会被丢弃。只有本函数为真时才
     * 可以推进时间轴，否则会跳。
     */
    bool isPlaybackActive() const { return m_playbackActive; }

    bool isBusy() const { return m_busy; }
    QString lastError() const { return m_lastError; }

    QString leftArmName() const { return m_leftArmName; }
    QString rightArmName() const { return m_rightArmName; }

    /** 由控制面板提供真实的臂名 */
    void setArmNames(const QString &left, const QString &right);
    /** 由控制面板持续提供实测关节角（入控首包基准） */
    void setActualJoints(const QString &armName, const QVector<double> &joints);

    quint16 commandPort() const { return m_commandPort; }
    void setCommandPort(quint16 port);

    /** 控制器地址。未显式设置时自动跟随 WebSocket 连接的主机 */
    QString hostAddress() const { return m_host; }
    void setHostAddress(const QString &host);
    /** 实际生效的 UDP 目标地址 */
    QString effectiveHost() const;

    int frequency() const { return m_frequency; }
    void setFrequency(int hz);

public slots:
    /** 入控：立刻起流发实测锚点，成功后等 pushFrame 送来轨迹起点 */
    void startRealtime();

    /** 停控：停流 + stopControl 交还通道 */
    void stopRealtime();

    /** PlaybackEngine 送来的一帧目标关节角 */
    void pushFrame(const JointState &state);

signals:
    void streamingStarted();
    void streamingStopped();

    /**
     * 入控过渡完成，首个轨迹目标已与实测角对齐。
     *
     * 过渡期间 pushFrame 送来的帧会被丢弃（机器人此刻还在从实测角
     * 平滑靠向轨迹起点）。若这时时间轴照常推进，过渡结束时轨迹已经
     * 跑到 700ms 之后，机器人会一步跳到那个位置——真机上就是明显的跳变。
     * 所以时间轴应当在收到本信号后才真正开始推进。
     */
    void leadInFinished();

    void armSessionChanged(const QString &armName, bool active);
    void errorOccurred(const QString &message);
    void statusMessage(const QString &message);
    /** 实际下发速率，用于诊断是否稳定 */
    void streamStats(int actualHz, qint64 totalPackets);

private slots:
    void onResponse(const QJsonObject &response);
    void onLeadInTimer();
    void onStreamStats(int actualHz, qint64 totalPackets);
    void checkStartupHealth();
    /** 抓取控制器会话诊断（receivedCount 等），判断包是否到达 */
    void probeSessionDiagnostics();
    void reportSessionDiagnostics(const QJsonObject &statusDb);

private:
    struct ArmSession {
        QString name;
        bool active = false;
        QVector<double> anchor;      // 入控瞬间的实测关节角（首包基准）
        QVector<double> leadFrom;
        QVector<double> leadTo;
        QVector<double> latest;      // 最新一帧目标
    };

    bool sendJoints(ArmSession &arm, const QVector<double> &joints);
    void sendStopControl();
    void resetSessions();
    /** 是否还有臂缺少实测关节角（入控首包基准） */
    bool missingActualJoints() const;
    /** 主动拉一次状态，补齐实测关节角 */
    void requestActualJoints();
    /** 把某臂当前应发的目标推给发送线程 */
    void updateStreamer(ArmSession &arm);
    /** 把当前生效的 host 推给发送线程 */
    void pushHostToStreamer();

    RobotConnection *m_connection = nullptr;

    JointStreamer *m_streamer = nullptr;
    QThread *m_streamThread = nullptr;

    QTimer *m_leadInTimer = nullptr;
    QTimer *m_startupWatchdog = nullptr;

    QString m_leftArmName;
    QString m_rightArmName;
    QVector<double> m_leftActual;
    QVector<double> m_rightActual;

    ArmSession m_left;
    ArmSession m_right;

    QString m_host = QStringLiteral("127.0.0.1");
    bool m_hostExplicit = false;        // 是否由用户显式指定，否则跟随 WebSocket 主机
    quint16 m_commandPort = 9030;
    int m_frequency = 100;              // 发包频率 Hz

    bool m_busy = false;
    bool m_streaming = false;
    bool m_leadInActive = false;
    bool m_playbackActive = false;
    bool m_waitingForJoints = false;

    int m_startRequestId = -1;
    int m_stopRequestId = -1;
    int m_statusRequestId = -1;
    int m_diagRequestId = -1;
    int m_probeRequestId = -1;
    bool m_statusProbed = false;        // 本次入控是否已查过指令端口
    bool m_cleanupDone = false;         // 本次入控是否已清过残留会话
    int m_cleanupRetries = 0;           // 清理重试次数，防止无限循环
    QString m_lastError;

    QElapsedTimer m_clock;
    double m_leadInStartMs = 0.0;
    double m_leadInDurationMs = 700.0;
};

#endif // REALTIME_CONTROLLER_H
