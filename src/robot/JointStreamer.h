#ifndef JOINTSTREAMER_H
#define JOINTSTREAMER_H

#include <QObject>
#include <QHash>
#include <QString>
#include <QVector>
#include <QElapsedTimer>

class QUdpSocket;
class QTimer;

/**
 * UDP 关节流发送器。运行在独立线程里。
 *
 * 为什么必须独立线程：
 * 控制器的「缺帧保持窗」只有 5 个锚点周期（下限 30ms），UI 线程一旦被
 * 3D 重绘、WebSocket 解析占住超过这个窗口，控制器就判定断流并回收直达通道
 * （现场表现就是「直达通道陈旧，已自动回收到规划通道」）。
 * 把发送放在专用线程，UI 卡顿不会让关节流断顿。
 *
 * 节流用「绝对下一次发送时刻」推进，不做累加复位，长时间运行不会漂移。
 */
class JointStreamer : public QObject
{
    Q_OBJECT

public:
    explicit JointStreamer(QObject *parent = nullptr);
    ~JointStreamer();

    /** 频率单位 Hz，允许 10~1000 */
    void setFrequency(int hz);
    int frequency() const { return m_frequency; }

    void setCommandPort(quint16 port);
    void setHost(const QString& host);

public slots:
    /** 开始按频率发送。目标尚未设置时不会发包。 */
    void start();
    void stop();

    /**
     * 更新某臂的目标关节角。发送线程按时间戳在相邻两次更新之间插值，
     * 因此这里给的间隔越均匀，下发越平滑（UI 的 16ms 帧率即可）。
     */
    void setTarget(const QString& armName, const QVector<double>& joints);

    /**
     * 静止多久后停发（ms）。0 表示不停发。
     *
     * 停在原地时反复下发同一个目标没有意义，而且会让伺服一直有微弱的
     * 位置重发激励。协议里 holdOnIdle 缺省为 true，停发不会掉线，
     * 所以静止后可以直接停，目标再变化时自动恢复。
     */
    void setIdleStopMs(int ms);
    int idleStopMs() const { return m_idleStopMs; }

    /** 移除某臂，不再为它发包 */
    void removeArm(const QString& armName);

signals:
    /** 供上层统计实际发送速率与排障 */
    void statsUpdated(int actualHz, qint64 totalPackets);
    /** 发送层异常（socket 失败、报文非法等），带原因 */
    void sendProblem(const QString& reason);

private slots:
    void onTick();

private:
    /** 单臂的目标锚点流。发送线程按时间戳在锚点之间插值。 */
    struct ArmStream {
        QVector<double> from;       ///< 上一个锚点
        QVector<double> to;         ///< 最新锚点
        qint64 fromNs = 0;
        qint64 toNs = 0;
        double intervalMs = 16.0;   ///< 观测到的锚点间隔（EMA），用于定插值延迟
        bool primed = false;

        /// 最近一次目标发生实质变化的时刻。用于判定「停在原地」。
        qint64 lastChangeNs = 0;
        /// 已判定静止且已停发。目标再次变化时立即解除。
        bool idle = false;
    };

    /** 在发送线程里创建 socket（QAbstractSocket 必须与使用它的线程同属） */
    void ensureSocket();
    void sendAll();
    bool writeOne(const QString& armName, const QVector<double>& joints);

    /**
     * 取 t = now - delay 时刻的关节角。
     *
     * 为什么要延迟一整个锚点间隔：
     * 上游按 16ms 更新目标，本线程按 10ms 发包。直接发「最新目标」会让
     * 同一个值连发一两包、再突然跳一步——控制器在两个锚点间线性插值，
     * 表现出来就是速度呈锯齿状脉动，伺服会发出高频噪音。
     * 延迟一个完整间隔后，采样点刚好扫过整个 [fromNs, toNs] 区间，
     * 每个发包周期都取到不同的值，速度就连续了。
     */
    QVector<double> sampleAt(const ArmStream& s, qint64 now) const;

    /** 是否所有臂都静止到可以停发（holdOnIdle 保证会话不会因此掉线） */
    bool allIdle(qint64 now) const;

    QUdpSocket* m_udp = nullptr;
    QTimer* m_timer = nullptr;

    QHash<QString, ArmStream> m_targets;

    QString m_host = QStringLiteral("127.0.0.1");
    quint16 m_port = 9030;
    int m_frequency = 100;

    QElapsedTimer m_clock;
    qint64 m_nextSendNs = 0;
    qint64 m_totalPackets = 0;
    int m_sendFailures = 0;
    qint64 m_statsWindowStartNs = 0;
    int m_statsWindowPackets = 0;
    bool m_loggedFirstPacket = false;
    bool m_warnedNonFinite = false;

    /// 静止多久后停发。缺省 1000ms：比任何一段动作的位移都长，
    /// 又短到不会让用户觉得「怎么还在发」。
    int m_idleStopMs = 1000;
    /// 判定「有实质变化」的阈值（rad）。比伺服分辨率大一个量级，
    /// 避免插值尾部那点数值噪声把静止判定反复破掉。
    static constexpr double kIdleEpsilon = 1e-5;
};

#endif // JOINTSTREAMER_H
