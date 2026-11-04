#include "robot/HandController.h"
#include "robot/RobotConnection.h"

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonValue>
#include <QTimer>
#include <QDebug>

namespace HandProtocol {

quint16 crc16(const QByteArray &data)
{
    quint16 crc = 0xFFFF;
    for (unsigned char byte : data) {
        crc ^= byte;
        for (int i = 0; i < 8; ++i) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

namespace {
/// body 是不含 CRC 的从站地址 + 功能码 + 数据
QByteArray withCrc(const QByteArray &body)
{
    QByteArray frame = body;
    const quint16 crc = crc16(body);
    frame.append(static_cast<char>(crc & 0xFF));
    frame.append(static_cast<char>((crc >> 8) & 0xFF));
    return frame;
}
} // namespace

QByteArray buildWriteAllFrame(int slave, const QVector<int> &positions, int timeMs)
{
    QByteArray body;
    body.append(static_cast<char>(slave & 0xFF));
    body.append(static_cast<char>(0x10));               // 功能码：写多个寄存器

    // 起始寄存器 1010，数量 12，字节数 24
    body.append(static_cast<char>((kRegWriteAll >> 8) & 0xFF));
    body.append(static_cast<char>(kRegWriteAll & 0xFF));
    body.append(static_cast<char>(0x00));
    body.append(static_cast<char>(0x0C));
    body.append(static_cast<char>(0x18));

    // 寄存器 1010~1021 按 (位置, 时长) 交替排列，每个 2 字节
    for (int i = 0; i < kDofCount; ++i) {
        const int pos = qBound(0, positions.value(i, 0), 1000);
        body.append(static_cast<char>((pos >> 8) & 0xFF));
        body.append(static_cast<char>(pos & 0xFF));
        body.append(static_cast<char>((timeMs >> 8) & 0xFF));
        body.append(static_cast<char>(timeMs & 0xFF));
    }
    return withCrc(body);
}

QByteArray buildReadPositionsFrame(int slave)
{
    QByteArray body;
    body.append(static_cast<char>(slave & 0xFF));
    body.append(static_cast<char>(0x04));               // 功能码：读输入寄存器
    body.append(static_cast<char>((kRegReadPos >> 8) & 0xFF));
    body.append(static_cast<char>(kRegReadPos & 0xFF));
    body.append(static_cast<char>(0x00));
    body.append(static_cast<char>(kDofCount));
    return withCrc(body);
}

bool verifyCrc(const QByteArray &frame)
{
    if (frame.size() < 4) {
        return false;
    }
    const quint16 crc = crc16(frame.left(frame.size() - 2));
    const quint16 expect =
        static_cast<quint16>((static_cast<unsigned char>(frame.at(frame.size() - 1)) << 8)
                             | static_cast<unsigned char>(frame.at(frame.size() - 2)));
    return crc == expect;
}

int degToRaw(double deg, int dof)
{
    if (dof < 0 || dof >= kDofCount) {
        return 0;
    }
    return qBound(0, qRound(deg / kMaxDeg[dof] * 1000.0), 1000);
}

double rawToDeg(int raw, int dof)
{
    if (dof < 0 || dof >= kDofCount) {
        return 0.0;
    }
    return qBound(0.0, raw / 1000.0 * kMaxDeg[dof], static_cast<double>(kMaxDeg[dof]));
}

const QStringList &dofNamesCn()
{
    static const QStringList kNames = {
        QStringLiteral("拇指弯曲"),
        QStringLiteral("拇指侧摆"),
        QStringLiteral("食指"),
        QStringLiteral("中指"),
        QStringLiteral("无名指"),
        QStringLiteral("小指"),
    };
    return kNames;
}

QStringList presetNames()
{
    return {QStringLiteral("一把抓"), QStringLiteral("半抓"), QStringLiteral("释放"),
            QStringLiteral("两指捏"), QStringLiteral("点赞"), QStringLiteral("指点")};
}

QVector<int> preset(const QString &name)
{
    static const QHash<QString, QVector<int>> kPresets = {
        {QStringLiteral("一把抓"), {1000, 0, 1000, 1000, 1000, 1000}},
        {QStringLiteral("释放"),   {0, 0, 0, 0, 0, 0}},
        {QStringLiteral("半抓"),   {500, 0, 500, 500, 500, 500}},
        {QStringLiteral("两指捏"), {800, 0, 200, 0, 0, 0}},
        {QStringLiteral("点赞"),   {0, 0, 1000, 1000, 1000, 1000}},
        {QStringLiteral("指点"),   {1000, 0, 0, 1000, 1000, 1000}},
    };
    return kPresets.value(name.trimmed());
}

} // namespace HandProtocol

// ============================================================================
// HandController
// ============================================================================

HandController::HandController(RobotConnection *connection, QObject *parent)
    : QObject(parent)
    , m_connection(connection)
{
    m_positionsLeft.fill(0, HandProtocol::kDofCount);
    m_positionsRight.fill(0, HandProtocol::kDofCount);
}

void HandController::setConnection(RobotConnection *connection)
{
    m_connection = connection;
    disconnectHands();
}

bool HandController::isReady() const
{
    return m_readyLeft && m_readyRight;
}

bool HandController::sideReady(bool left) const
{
    return left ? m_readyLeft : m_readyRight;
}

QVector<int> HandController::positions(bool left) const
{
    return left ? m_positionsLeft : m_positionsRight;
}

void HandController::setLocalPositions(bool left, const QVector<int> &positions)
{
    QVector<int> clamped;
    clamped.reserve(HandProtocol::kDofCount);
    for (int i = 0; i < HandProtocol::kDofCount; ++i) {
        clamped.append(qBound(0, positions.value(i, 0), 1000));
    }
    if (left) {
        m_positionsLeft = clamped;
    } else {
        m_positionsRight = clamped;
    }
    emit positionsChanged();
}

void HandController::setWriteTimeMs(int ms)
{
    m_writeTimeMs = qBound(1, ms, 2000);
}

// ---------------------------------------------------------------- 收发

QJsonObject HandController::call(const QString &ty, const QJsonObject &db, int timeoutMs)
{
    if (!m_connection || !m_connection->isConnected()) {
        return {};
    }

    const int id = m_connection->sendCommand(ty, db);
    if (id < 0) {
        return {};
    }

    // 调用式接口是请求—响应，没有异步回调可挂，只能就地等这一条响应。
    // 用局部事件循环：主线程在此期间仍能处理 socket 信号，不会卡死界面。
    QEventLoop loop;
    QJsonObject result;
    bool got = false;

    auto conn = connect(m_connection, &RobotConnection::responseReceived, this,
                        [&](const QJsonObject &obj) {
                            if (obj.value(QStringLiteral("id")).toInt(-1) != id) {
                                return;
                            }
                            result = obj;
                            got = true;
                            loop.quit();
                        });

    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(qMax(200, timeoutMs));

    loop.exec();
    disconnect(conn);

    if (!got) {
        qWarning() << "HandController:" << ty << "无响应（超时" << timeoutMs << "ms）";
        return {};
    }
    return result;
}

QString HandController::resolveChannel(const QString &slaveName, QString *channel)
{
    const QJsonObject resp = call(QStringLiteral("rs485/list"), QJsonObject(), 5000);
    if (resp.isEmpty()) {
        return QStringLiteral("rs485/list 无响应");
    }
    if (resp.contains(QStringLiteral("err"))) {
        return QStringLiteral("rs485/list 失败: %1")
            .arg(resp.value(QStringLiteral("err")).toObject()
                     .value(QStringLiteral("message")).toString());
    }

    const QJsonArray chans = resp.value(QStringLiteral("db")).toArray();
    QStringList names;
    for (const QJsonValue &v : chans) {
        const QJsonObject c = v.toObject();
        const QString ch = c.value(QStringLiteral("channel")).toString();
        names << ch;

        const QString slave = c.value(QStringLiteral("backendStatus")).toObject()
                                  .value(QStringLiteral("slave")).toString();
        if (slave == slaveName || ch == slaveName
            || ch.section(QLatin1Char('/'), -1) == slaveName) {
            if (c.contains(QStringLiteral("supported"))
                && !c.value(QStringLiteral("supported")).toBool()) {
                return QStringLiteral("通道 %1 当前平台不支持").arg(ch);
            }
            if (channel) {
                *channel = ch;
            }
            return {};
        }
    }

    return QStringLiteral("rs485/list 中未找到从站 %1（现有: %2）")
        .arg(slaveName, names.isEmpty() ? QStringLiteral("无") : names.join(QStringLiteral(", ")));
}

QString HandController::initChannel(const QString &channel)
{
    QJsonObject db;
    db[QStringLiteral("channel")] = channel;
    db[QStringLiteral("baudRate")] = 115200;
    db[QStringLiteral("stopBits")] = 1;
    db[QStringLiteral("parity")] = 0;

    const QJsonObject resp = call(QStringLiteral("rs485/init"), db, 6000);
    if (resp.isEmpty()) {
        return QStringLiteral("rs485/init 无响应");
    }
    if (resp.contains(QStringLiteral("err"))) {
        const QJsonObject err = resp.value(QStringLiteral("err")).toObject();
        return QStringLiteral("rs485/init 失败 (%1): %2")
            .arg(err.value(QStringLiteral("code")).toInt())
            .arg(err.value(QStringLiteral("message")).toString());
    }
    return {};
}

QByteArray HandController::writeRead(const QString &channel, const QByteArray &frame, int readLength)
{
    QJsonArray bytes;
    for (unsigned char c : frame) {
        bytes.append(static_cast<int>(c));
    }
    QJsonObject wdb;
    wdb[QStringLiteral("channel")] = channel;
    wdb[QStringLiteral("data")] = bytes;

    const QJsonObject wresp = call(QStringLiteral("rs485/write"), wdb, 3000);
    if (wresp.isEmpty()) {
        return {};
    }
    if (wresp.contains(QStringLiteral("err"))) {
        return {};
    }

    if (readLength <= 0) {
        return QByteArray(1, '\0');     // 只发不读：用一个非空标记表示写成功
    }

    QJsonObject rdb;
    rdb[QStringLiteral("channel")] = channel;
    rdb[QStringLiteral("timeout")] = 400;
    rdb[QStringLiteral("length")] = readLength;

    const QJsonObject rresp = call(QStringLiteral("rs485/read"), rdb, 1500);
    if (rresp.isEmpty() || rresp.contains(QStringLiteral("err"))) {
        return {};
    }

    const QJsonValue dbv = rresp.value(QStringLiteral("db"));
    if (!dbv.isArray()) {
        return {};
    }

    QByteArray rx;
    const QJsonArray arr = dbv.toArray();
    rx.reserve(arr.size());
    for (const QJsonValue &bv : arr) {
        rx.append(static_cast<char>(bv.toInt(0) & 0xFF));
    }
    return rx;
}

// ---------------------------------------------------------------- 业务

void HandController::connectHands()
{
    if (!m_connection || !m_connection->isConnected()) {
        emit errorOccurred(QStringLiteral("请先连接机械臂，灵巧手经控制器 rs485 转发"));
        return;
    }

    m_readyLeft = false;
    m_readyRight = false;

    // 左
    emit statusMessage(QStringLiteral("正在初始化左手…"));
    QString channel;
    QString err = resolveChannel(QString::fromLatin1(HandProtocol::kSlaveNameLeft), &channel);
    if (err.isEmpty()) {
        err = initChannel(channel);
        if (err.isEmpty()) {
            m_channelLeft = channel;
            m_readyLeft = true;
        }
    }
    if (!err.isEmpty()) {
        qWarning() << "HandController: 左手初始化失败" << err;
        emit errorOccurred(QStringLiteral("左手初始化失败：%1").arg(err));
    }

    // 右
    emit statusMessage(QStringLiteral("正在初始化右手…"));
    channel.clear();
    err = resolveChannel(QString::fromLatin1(HandProtocol::kSlaveNameRight), &channel);
    if (err.isEmpty()) {
        err = initChannel(channel);
        if (err.isEmpty()) {
            m_channelRight = channel;
            m_readyRight = true;
        }
    }
    if (!err.isEmpty()) {
        qWarning() << "HandController: 右手初始化失败" << err;
        emit errorOccurred(QStringLiteral("右手初始化失败：%1").arg(err));
    }

    const bool ready = isReady();
    if (ready) {
        emit statusMessage(QStringLiteral("灵巧手已就绪"));
        refreshPositions();
    } else if (m_readyLeft || m_readyRight) {
        emit statusMessage(QStringLiteral("灵巧手部分就绪（%1）")
                               .arg(m_readyLeft ? QStringLiteral("左") : QStringLiteral("右")));
    }
    emit readyChanged(ready);
}

void HandController::disconnectHands()
{
    // 串口保持控制器那边的状态不动：灵巧手是独立外设，断开只表示本程序
    // 不再下发，没必要去 reset 通道，否则下次连接又要重走一遍 init。
    m_readyLeft = false;
    m_readyRight = false;
    m_channelLeft.clear();
    m_channelRight.clear();
    emit readyChanged(false);
}

bool HandController::sendSide(bool left, const QVector<int> &positions)
{
    const QString channel = left ? m_channelLeft : m_channelRight;
    const bool ready = left ? m_readyLeft : m_readyRight;
    if (!ready || channel.isEmpty()) {
        return false;
    }

    const int slave = left ? HandProtocol::kSlaveAddrLeft : HandProtocol::kSlaveAddrRight;
    const QByteArray frame =
        HandProtocol::buildWriteAllFrame(slave, positions, m_writeTimeMs);

    // 写应答固定 8 字节：从站 + 功能码 + 起始寄存器(2) + 数量(2) + CRC(2)
    const QByteArray rx = writeRead(channel, frame, 8);
    if (rx.isEmpty()) {
        setError(QStringLiteral("%1手无应答").arg(left ? QStringLiteral("左") : QStringLiteral("右")));
        return false;
    }
    if (!HandProtocol::verifyCrc(rx)) {
        setError(QStringLiteral("%1手应答 CRC 错误").arg(left ? QStringLiteral("左") : QStringLiteral("右")));
        return false;
    }
    if (rx.size() >= 3 && (static_cast<unsigned char>(rx.at(1)) & 0xFF) == 0x90) {
        setError(QStringLiteral("%1手从站异常码 0x%2")
                     .arg(left ? QStringLiteral("左") : QStringLiteral("右"))
                     .arg(static_cast<unsigned char>(rx.at(2)), 2, 16, QChar('0')));
        return false;
    }
    return true;
}

void HandController::sendBoth(const QVector<int> &left, const QVector<int> &right)
{
    // 没有启用开关：能不能下发完全由「连接是否就绪」决定，
    // 下面按 left/right 各自的 ready 状态分别判断。
    bool ok = true;
    if (m_readyLeft) {
        if (sendSide(true, left)) {
            setLocalPositions(true, left);
        } else {
            ok = false;
        }
    }
    if (m_readyRight) {
        if (sendSide(false, right)) {
            setLocalPositions(false, right);
        } else {
            ok = false;
        }
    }

    if (ok) {
        emit statusMessage(QStringLiteral("灵巧手指令已下发"));
    }
}

void HandController::refreshPositions()
{
    for (int side = 0; side < 2; ++side) {
        const bool left = (side == 0);
        const QString channel = left ? m_channelLeft : m_channelRight;
        if (!(left ? m_readyLeft : m_readyRight) || channel.isEmpty()) {
            continue;
        }

        const int slave = left ? HandProtocol::kSlaveAddrLeft : HandProtocol::kSlaveAddrRight;
        const QByteArray frame = HandProtocol::buildReadPositionsFrame(slave);
        // 应答：从站 + 功能码 + 字节数(12) + 12 字节数据 + CRC(2) = 17
        const QByteArray rx = writeRead(channel, frame, 17);
        if (rx.size() < 17 || !HandProtocol::verifyCrc(rx)) {
            continue;
        }

        QVector<int> pos;
        pos.reserve(HandProtocol::kDofCount);
        for (int i = 0; i < HandProtocol::kDofCount; ++i) {
            const int hi = static_cast<unsigned char>(rx.at(3 + i * 2));
            const int lo = static_cast<unsigned char>(rx.at(4 + i * 2));
            pos.append(qBound(0, (hi << 8) | lo, 1000));
        }
        setLocalPositions(left, pos);
    }
}

void HandController::setError(const QString &message)
{
    qWarning() << "HandController:" << message;
    emit errorOccurred(message);
}
