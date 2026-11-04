#ifndef HANDCONTROLLER_H
#define HANDCONTROLLER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QJsonObject>

class RobotConnection;

/**
 * Revo2 灵巧手协议层（Modbus RTU over 控制器 rs485 接口）。
 *
 * 灵巧手不挂在 EtherCAT 上，而是接在末端通信板的 RS485 口，需要经控制器的
 * rs485/* 接口转发：
 *    rs485/list  → 按从站名找到 channel
 *    rs485/init  → 配置串口（115200 8N1）
 *    rs485/write → 写 Modbus 帧
 *    rs485/read  → 读应答
 *
 * 6 个自由度：拇指弯曲、拇指侧摆、食指、中指、无名指、小指。
 * 每个 DOF 归一化为 0~1000，各自对应不同的物理最大角度。
 */
namespace HandProtocol {

constexpr int kDofCount = 6;

/// 各 DOF 最大角度（度），0~1000 归一化值与之线性对应
constexpr int kMaxDeg[kDofCount] = {59, 90, 81, 81, 81, 81};

/// 默认 Modbus 从站地址
constexpr int kSlaveAddrLeft  = 126;
constexpr int kSlaveAddrRight = 127;

/// 末端通信板 EtherCAT 从站名
constexpr const char *kSlaveNameLeft  = "EtherCAT485L";
constexpr const char *kSlaveNameRight = "EtherCAT485R";

/// 寄存器：写全部 6 DOF / 读全部 6 DOF 位置
constexpr quint16 kRegWriteAll = 0x03F2;
constexpr quint16 kRegReadPos  = 0x07D0;

/// 写动作时长（ms），驱动器按这个时间走完目标，避免瞬间打到目标
constexpr int kDefaultWriteTimeMs = 100;

quint16 crc16(const QByteArray &data);

/// 写全部 6 DOF 的 Modbus 帧（功能码 0x10，寄存器 1010~1021）
QByteArray buildWriteAllFrame(int slave, const QVector<int> &positions, int timeMs);
/// 读全部 6 DOF 位置的 Modbus 帧（功能码 0x04）
QByteArray buildReadPositionsFrame(int slave);
/// 校验帧尾 CRC
bool verifyCrc(const QByteArray &frame);

int degToRaw(double deg, int dof);
double rawToDeg(int raw, int dof);

/// 各 DOF 中文名
const QStringList &dofNamesCn();
/// 预设手势名（中文）
QStringList presetNames();
/// 预设手势对应的归一化值；未知名称返回空
QVector<int> preset(const QString &name);

} // namespace HandProtocol

/**
 * 双手灵巧手客户端。
 *
 * 与机械臂连接相互独立：机械臂 WebSocket 只需要通，灵巧手还要单独「连接」
 * （解析通道 + 初始化串口）才能真正下发。所以界面上有独立的连接按钮，
 * 不连灵巧手也照样能规划和播放机械臂动作。
 */
class HandController : public QObject
{
    Q_OBJECT

public:
    explicit HandController(RobotConnection *connection, QObject *parent = nullptr);

    /** 复用同一条 WebSocket，但要求已连接机械臂 */
    void setConnection(RobotConnection *connection);

    /** 双手是否都已初始化成功。连接成功即代表要控制手，不再有单独的启用开关 */
    bool isReady() const;
    bool sideReady(bool left) const;

    /** 最近一次已知的手位（下发目标或读回值），归一化 0~1000 */
    QVector<int> positions(bool left) const;
    void setLocalPositions(bool left, const QVector<int> &positions);

    /** 写动作时长（ms） */
    int writeTimeMs() const { return m_writeTimeMs; }
    void setWriteTimeMs(int ms);

public slots:
    /** 解析通道 + init 串口 */
    void connectHands();
    /** 只清本地状态，不向控制器发任何东西 */
    void disconnectHands();
    /** 读回双手当前位置 */
    void refreshPositions();

    /**
     * 下发一组双手目标。只发一次，不做插值——段内的连续性是机械臂的事，
     * 灵巧手到位即结束。
     */
    void sendBoth(const QVector<int> &left, const QVector<int> &right);

signals:
    void readyChanged(bool ready);
    void statusMessage(const QString &message);
    void errorOccurred(const QString &message);
    /** 手位更新（下发成功或读回），供界面刷新 */
    void positionsChanged();

private:
    /** 同步发一条调用式命令，等待同 id 的响应（嵌套事件循环 + 超时） */
    QJsonObject call(const QString &ty, const QJsonObject &db, int timeoutMs);

    /** rs485/list 里按从站名定位 channel */
    QString resolveChannel(const QString &slaveName, QString *channel);
    /** rs485/init */
    QString initChannel(const QString &channel);
    /** rs485/write + rs485/read */
    QByteArray writeRead(const QString &channel, const QByteArray &frame, int readLength);

    bool sendSide(bool left, const QVector<int> &positions);
    void setError(const QString &message);

    RobotConnection *m_connection = nullptr;
    int m_writeTimeMs = HandProtocol::kDefaultWriteTimeMs;

    QString m_channelLeft;
    QString m_channelRight;
    bool m_readyLeft = false;
    bool m_readyRight = false;

    QVector<int> m_positionsLeft;
    QVector<int> m_positionsRight;
};

#endif // HANDCONTROLLER_H
