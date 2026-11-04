#include "robot/JointStreamer.h"

#include <QUdpSocket>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>
#include <QDebug>
#include <cmath>

namespace {
constexpr int kJointCount = 7;

QJsonArray toArray(const QVector<double> &v)
{
    QJsonArray a;
    for (double d : v) {
        a.append(d);
    }
    return a;
}
} // namespace

JointStreamer::JointStreamer(QObject *parent)
    : QObject(parent)
{
    // socket 不在这里创建：QAbstractSocket 必须在真正使用它的线程里构造，
    // 构造函数跑在主线程，而本对象随后会被 moveToThread。
    m_timer = new QTimer(this);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &JointStreamer::onTick);
    m_clock.start();
}

JointStreamer::~JointStreamer()
{
}

void JointStreamer::ensureSocket()
{
    if (m_udp) {
        return;
    }
    m_udp = new QUdpSocket(this);
    // 不 bind：发送方用临时端口即可，控制器按源地址回推状态推送（本类不订阅）
    qDebug() << "JointStreamer: socket 已创建于发送线程";
}

void JointStreamer::setFrequency(int hz)
{
    // 协议允许 10~1000Hz
    m_frequency = qBound(10, hz, 1000);
    if (m_timer->isActive()) {
        // 粒度必须跟着周期走，否则升频后 tick 跟不上周期，
        // 实际下发频率会被卡在 tick 频率上（例如 100→300Hz 只跑到 200Hz）。
        const int periodMs = qMax(1, 1000 / qMax(1, m_frequency));
        m_timer->setInterval(qMax(1, periodMs / 2));
        // 周期变了，重新对齐下一次发送时刻，避免用旧节拍发一包
        m_nextSendNs = m_clock.nsecsElapsed();
    }
}

void JointStreamer::setCommandPort(quint16 port)
{
    m_port = port;
}

void JointStreamer::setHost(const QString &host)
{
    if (!host.isEmpty()) {
        m_host = host;
    }
}

void JointStreamer::setIdleStopMs(int ms)
{
    m_idleStopMs = qBound(0, ms, 60000);
    if (m_idleStopMs == 0) {
        // 关掉静止停发：所有臂立刻恢复，否则会一直停在「已停止」状态
        for (auto it = m_targets.begin(); it != m_targets.end(); ++it) {
            it.value().idle = false;
        }
    }
}

void JointStreamer::start()
{
    if (m_timer->isActive()) {
        return;
    }

    ensureSocket();

    m_clock.restart();
    m_totalPackets = 0;
    m_sendFailures = 0;
    m_loggedFirstPacket = false;
    m_warnedNonFinite = false;
    m_statsWindowPackets = 0;
    m_statsWindowStartNs = 0;
    m_nextSendNs = 0;

    // 入控时 setTarget 先于 start 被调用，那时记的时间戳基于旧时钟。
    // restart 后必须重新打点，否则 fromNs/toNs 落在未来或远早于现在，
    // sampleAt 的区间判定不成立，插值等于没开。
    const qint64 nowNs = m_clock.nsecsElapsed();
    for (auto it = m_targets.begin(); it != m_targets.end(); ++it) {
        ArmStream &s = it.value();
        s.fromNs = nowNs;
        s.toNs = nowNs;
        s.lastChangeNs = nowNs;     // 起流时不算静止，先正常发一包
        s.idle = false;
        s.from = s.to;          // 首个锚点自己和自己插值，保持不动
        s.primed = !s.to.isEmpty();
    }

    // 定时器粒度取半个周期，保证每周期都有机会命中；
    // 真正的节拍由 onTick 里的绝对时刻控制。
    const int periodMs = qMax(1, 1000 / qMax(1, m_frequency));
    const int tickMs = qMax(1, periodMs / 2);
    m_timer->setInterval(tickMs);
    m_timer->start();

    qDebug() << "JointStreamer: 启动，" << m_frequency << "Hz，定时器粒度"
             << tickMs << "ms，目标" << m_host << ":" << m_port;
}

void JointStreamer::stop()
{
    if (m_timer->isActive()) {
        m_timer->stop();
    }
    m_targets.clear();
}

void JointStreamer::setTarget(const QString &armName, const QVector<double> &joints)
{
    if (armName.isEmpty() || joints.size() < kJointCount) {
        return;
    }

    ArmStream &s = m_targets[armName];
    const qint64 now = m_clock.nsecsElapsed();

    if (!s.primed) {
        // 首帧：上下锚点同值，避免刚起流就从一个空值插值过去
        s.from = joints;
        s.to = joints;
        s.fromNs = now;
        s.toNs = now;
        s.lastChangeNs = now;
        s.idle = false;
        s.primed = true;
        return;
    }

    // 判断目标是否真的变了。上游在静止时会持续送来同一个值，
    // 这些「没有位移的更新」不该被当成运动，否则永远判不出静止。
    bool moved = false;
    for (int i = 0; i < kJointCount; ++i) {
        if (qAbs(joints.value(i, 0.0) - s.to.value(i, 0.0)) > kIdleEpsilon) {
            moved = true;
            break;
        }
    }

    // 统计锚点间隔，用于决定插值延迟该给多少。
    // 用 EMA 而不是取最近一次：上游偶尔卡一帧不会让延迟突然抖一下。
    const double dtMs = (now - s.toNs) / 1e6;
    if (dtMs > 1.0 && dtMs < 200.0) {
        s.intervalMs = 0.85 * s.intervalMs + 0.15 * dtMs;
    }

    if (moved) {
        s.from = s.to;
        s.fromNs = s.toNs;
        s.to = joints;
        s.toNs = now;
        s.lastChangeNs = now;
        // 一旦重新动起来就立刻恢复下发
        s.idle = false;
    }
    // 没动时什么都不做。
    //
    // 特别注意不能把 toNs 推到 now：那会把插值区间越拉越长，
    // 采样点被延迟后落在区间内部，输出就会朝 to 缓慢爬行——
    // 本该静止的关节反而在持续微动，正好是我们要消除的那种噪声。
    // 保持 toNs 不动，采样时刻自然越过它，sampleAt 直接返回 to。
}

QVector<double> JointStreamer::sampleAt(const ArmStream &s, qint64 now) const
{
    if (!s.primed) {
        return {};
    }

    // 延迟一整个锚点间隔。
    //
    // 必须是一整个而不是半个：锚点在 [fromNs, toNs] 之间才有插值区间，
    // 若只延迟半个间隔，则每个 16ms 窗口里只有前一半落在区间内、
    // 后一半 t 已经越过 toNs 被钉住，仍然是「走一下、停一下」的阶梯。
    // 延迟一整个间隔，t 扫过的正好是完整的 [fromNs, toNs]，
    // 每个发包周期都取到不同的值，速度连续。
    const qint64 delayNs = qint64(s.intervalMs * 1e6);
    const qint64 t = now - delayNs;

    const qint64 span = s.toNs - s.fromNs;
    if (span <= 0) {
        return s.to;
    }

    // 超出当前区间（上游还没送来新锚点）：钉在端点，不外推。
    // 外推会让机械臂朝一个尚未确定的方向继续走。
    if (t >= s.toNs) {
        return s.to;
    }
    if (t <= s.fromNs) {
        return s.from;
    }

    const double u = double(t - s.fromNs) / double(span);
    QVector<double> r(kJointCount, 0.0);
    for (int i = 0; i < kJointCount; ++i) {
        const double a = s.from.value(i, 0.0);
        const double b = s.to.value(i, 0.0);
        r[i] = a + u * (b - a);
    }
    return r;
}

void JointStreamer::removeArm(const QString &armName)
{
    m_targets.remove(armName);
}

bool JointStreamer::allIdle(qint64 now) const
{
    if (m_idleStopMs <= 0) {
        return false;       // 没开启静止停发
    }
    if (m_targets.isEmpty()) {
        return false;       // 交给 onTick 顶部的判空处理
    }

    const qint64 holdNs = qint64(m_idleStopMs) * 1000000LL;
    for (auto it = m_targets.constBegin(); it != m_targets.constEnd(); ++it) {
        const ArmStream &s = it.value();
        if (!s.primed) {
            return false;
        }
        if (s.idle) {
            continue;       // 已经停发的臂不再参与判定
        }
        if (now - s.lastChangeNs < holdNs) {
            return false;   // 还没到静止门限
        }
    }
    return true;
}

void JointStreamer::onTick()
{
    if (m_targets.isEmpty()) {
        return;
    }

    // 绝对时刻推进：把「下一次该发的时刻」按周期往后推，
    // 而不是每 tick 累加再归零。后者在定时器粒度不是周期整数倍时会漂移。
    const qint64 periodNs = 1000000000LL / qMax(1, m_frequency);
    const qint64 now = m_clock.nsecsElapsed();

    if (now < m_nextSendNs) {
        return;
    }

    // 如果落后超过一整个周期（例如系统卡了一下），直接对齐到当前时刻，
    // 不去补发积压的包——补发只会把旧目标灌给控制器。
    if (now - m_nextSendNs > periodNs * 2) {
        m_nextSendNs = now;
    }
    m_nextSendNs += periodNs;

    // 停在原地且已超过静止门限：停发。
    //
    // 协议 holdOnIdle 缺省 true，停发不会掉线——会话保持 active、通道仍在
    // 直达侧，机器人停在最后一个目标上，再发目标即继续跟随。
    // 持续按 100Hz 重发同一个位置对静止的伺服毫无作用，反而让驱动器
    // 一直处在「收到新指令」的状态里。
    if (allIdle(now)) {
        bool firstStop = false;
        for (auto it = m_targets.begin(); it != m_targets.end(); ++it) {
            if (!it.value().idle) {
                it.value().idle = true;
                firstStop = true;
            }
        }
        if (firstStop) {
            qDebug() << "JointStreamer: 已静止" << m_idleStopMs
                     << "ms，暂停下发（holdOnIdle 保持会话，目标变化即恢复）";
        }
        return;
    }

    sendAll();

    // 每秒汇报一次实际频率，便于判断有没有真的跑到设定值
    if (m_statsWindowStartNs == 0) {
        m_statsWindowStartNs = now;
    } else if (now - m_statsWindowStartNs >= 1000000000LL) {
        const double secs = (now - m_statsWindowStartNs) / 1e9;
        const int hz = int(m_statsWindowPackets / qMax(1e-6, secs));
        emit statsUpdated(hz, m_totalPackets);
        m_statsWindowStartNs = now;
        m_statsWindowPackets = 0;
    }
}

void JointStreamer::sendAll()
{
    const qint64 now = m_clock.nsecsElapsed();
    for (auto it = m_targets.constBegin(); it != m_targets.constEnd(); ++it) {
        const QVector<double> v = sampleAt(it.value(), now);
        if (v.isEmpty()) {
            continue;
        }
        if (writeOne(it.key(), v)) {
            ++m_totalPackets;
            ++m_statsWindowPackets;
        }
    }
}

bool JointStreamer::writeOne(const QString &armName, const QVector<double> &joints)
{
    if (joints.size() < kJointCount) {
        return false;
    }

    // 非有限值（NaN/Inf）会被控制器整包丢弃并置 invalidDatagram。
    // 引擎在某些插值边界可能产生 NaN，这里挡掉并提示，避免"发了很多包但全被丢弃"。
    for (double v : joints) {
        if (!std::isfinite(v)) {
            if (!m_warnedNonFinite) {
                m_warnedNonFinite = true;
                qWarning() << "JointStreamer:" << armName
                           << "关节角含非有限值，控制器会整包丢弃:" << joints;
                emit sendProblem(QStringLiteral("关节角含 NaN/Inf，控制器会整包丢弃"));
            }
            return false;
        }
    }

    // UDP 报文是扁平 JSON，没有 {id, ty, db} 包裹；
    // value 长度必须等于该臂关节数，多了少了都整包丢弃。
    QJsonObject packet;
    packet["robot"] = armName;
    packet["controlSpace"] = 0;    // 关节空间
    packet["commandMode"] = 0;     // 绝对量
    packet["value"] = toArray(joints);

    const QByteArray payload = QJsonDocument(packet).toJson(QJsonDocument::Compact);
    const qint64 written = m_udp->writeDatagram(
        payload, QHostAddress(m_host), m_port);

    if (written < 0) {
        ++m_sendFailures;
        if (m_sendFailures <= 3) {
            qWarning() << "JointStreamer: UDP 发送失败" << m_udp->errorString()
                       << "目标" << m_host << m_port;
        }
        return false;
    }

    // 打出第一包的真实报文，联调时可直接核对格式与目标地址
    if (!m_loggedFirstPacket) {
        m_loggedFirstPacket = true;
        qDebug().noquote() << "JointStreamer: 首包 ->" << m_host << ":" << m_port
                           << QString::fromUtf8(payload);
    }
    return true;
}
