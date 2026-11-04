#include "ui/RobotControlPanel.h"
#include "ui_RobotControlPanel.h"
#include "ui/MessageCenter.h"
#include "robot/HandController.h"
#include <QComboBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QTimer>
#include <QDebug>

namespace {
/**
 * 连续点动的心跳超时（ms），作为 jogStart 的 timeoutMs 下发。
 *
 * 停顿超过这个时长控制器就平滑减速停住，属于「松手即停」的兜底。
 * 所以它同时决定了界面卡顿时点动还能撑多久。
 */
constexpr int kJogTimeoutMs = 500;

/**
 * 心跳发送周期（ms）。
 *
 * 协议要求发送间隔小于 timeoutMs，建议不超过其一半（500ms 超时对应 200~250ms）。
 * 原来固定 400ms 只剩 100ms 余量，UI 稍一卡顿就被判超时自动停，
 * 用户感觉是「长按走一下就断」。
 */
constexpr int kJogHeartbeatMs = 200;

/**
 * 遇到 20319（上一轮点动还在减速）时的重试策略。
 *
 * jogStop 是受控停：倍率平滑降到 0 才算结束。刚松手立刻再按，
 * 上一次会话还没关，jogStart 会被拒。等一小会儿再发即可，
 * 不该让用户看到「点了没反应」。
 */
constexpr int kJogRetryDelayMs = 120;
constexpr int kJogMaxRetries = 8;
}

RobotControlPanel::RobotControlPanel(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::RobotControlPanel)
    , m_connection(new RobotConnection(this))
    , m_jogHeartbeatTimer(new QTimer(this))
    , m_jogRetryTimer(new QTimer(this))
{
    ui->setupUi(this);
    
    m_leftJoints.resize(7);
    m_leftJoints.fill(0.0);
    m_rightJoints.resize(7);
    m_rightJoints.fill(0.0);

    // 灵巧手复用同一条 WebSocket：rs485/* 走的是控制器的调用式接口，
    // 但入控（rs485/list → init）与机械臂 realtime 完全独立，所以要单独连接。
    m_hand = new HandController(m_connection, this);
    
    // Setup connections
    connect(ui->btnConnect, &QPushButton::clicked, 
            this, &RobotControlPanel::onConnectClicked);
    connect(ui->btnToggleLeft, &QPushButton::clicked, 
            this, &RobotControlPanel::onToggleLeftClicked);
    connect(ui->btnToggleRight, &QPushButton::clicked, 
            this, &RobotControlPanel::onToggleRightClicked);
    connect(ui->btnGetCurrentAnglesLeft, &QPushButton::clicked, 
            this, &RobotControlPanel::onGetCurrentAnglesLeftClicked);
    connect(ui->btnGetCurrentAnglesRight, &QPushButton::clicked, 
            this, &RobotControlPanel::onGetCurrentAnglesRightClicked);

    // 灵巧手
    connect(ui->btnConnectHand, &QPushButton::clicked,
            this, &RobotControlPanel::onConnectHandClicked);
    connect(ui->btnRefreshHand, &QPushButton::clicked,
            this, &RobotControlPanel::onRefreshHandClicked);
    connect(ui->btnApplyHandPreset, &QPushButton::clicked,
            this, &RobotControlPanel::onApplyHandPresetClicked);
    connect(m_hand, &HandController::readyChanged, this, [this](bool ready) {
        updateHandUi();
        ui->labelHandStatus->setText(ready ? QStringLiteral("● 左手+右手已就绪")
                                           : QStringLiteral("● 未连接"));
    });
    connect(m_hand, &HandController::errorOccurred, this, [this](const QString &msg) {
        ui->labelHandStatus->setText(QStringLiteral("● %1").arg(msg));
        ui->labelHandStatus->setStyleSheet(
            QStringLiteral("QLabel { color:#E94560; background-color:#1E2636;"
                           " border-radius:4px; padding:6px; font-size:9pt; }"));
    });
    connect(m_hand, &HandController::statusMessage, this, [this](const QString &msg) {
        qInfo() << "[灵巧手]" << msg;
        ui->labelHandStatus->setText(QStringLiteral("● %1").arg(msg));
    });
    
    // RobotConnection signals
    connect(m_connection, &RobotConnection::connected, 
            this, &RobotControlPanel::onConnected);
    connect(m_connection, &RobotConnection::disconnected, 
            this, &RobotControlPanel::onDisconnected);
    connect(m_connection, &RobotConnection::robotStateUpdated, 
            this, &RobotControlPanel::onRobotStateUpdated);
    connect(m_connection, &RobotConnection::robotListReceived, 
            this, &RobotControlPanel::onRobotListReceived);
    connect(m_connection, &RobotConnection::responseReceived, 
            this, &RobotControlPanel::onResponseReceived);
    
    // Jog heartbeat timer
    // 心跳周期取 jogStart 所设 timeoutMs 的一半以内，留出网络抖动余量。
    // 原来固定 400ms 对 500ms 超时几乎没余量，一次卡顿就被判超时自动停，
    // 表现为「长按走两下就停」。
    m_jogHeartbeatTimer->setInterval(kJogHeartbeatMs);
    connect(m_jogHeartbeatTimer, &QTimer::timeout, [this]() {
        if (!m_activeJogArm.isEmpty()) {
            m_connection->jogHeartbeat(m_activeJogArm);
        }
    });

    // 上一轮点动还在减速时（20319）延后重发 jogStart
    m_jogRetryTimer->setSingleShot(true);
    m_jogRetryTimer->setInterval(kJogRetryDelayMs);
    connect(m_jogRetryTimer, &QTimer::timeout, this, [this]() {
        // 期间用户可能已经松手了，那就别再启动
        if (!m_activeJogArm.isEmpty()) {
            sendJogStart();
        }
    });
    
    createJointControls();
    createHandControls();
    updateHandUi();
    
    ui->btnToggleLeft->setEnabled(false);
    ui->btnToggleRight->setEnabled(false);
}

RobotControlPanel::~RobotControlPanel()
{
    delete ui;
}

void RobotControlPanel::createJointControls()
{
    // Create left arm joint controls
    QVBoxLayout* leftLayout = qobject_cast<QVBoxLayout*>(
        ui->scrollAreaWidgetContents_left->layout());
    if (!leftLayout) {
        leftLayout = new QVBoxLayout(ui->scrollAreaWidgetContents_left);
    }
    
    for (int i = 0; i < 7; ++i) {
        QWidget* jointWidget = new QWidget();
        QHBoxLayout* layout = new QHBoxLayout(jointWidget);
        
        QLabel* label = new QLabel(QString("J%1: 0.000").arg(i+1));
        label->setMinimumWidth(100);
        m_leftJointLabels.append(label);
        
        QPushButton* minusBtn = new QPushButton("-");
        minusBtn->setMaximumWidth(40);
        m_leftJogMinusButtons.append(minusBtn);
        connect(minusBtn, &QPushButton::pressed, [this, i]() {
            onJogButtonPressed("left", i, -1);
        });
        connect(minusBtn, &QPushButton::released, 
                this, &RobotControlPanel::onJogButtonReleased);
        
        QPushButton* plusBtn = new QPushButton("+");
        plusBtn->setMaximumWidth(40);
        m_leftJogPlusButtons.append(plusBtn);
        connect(plusBtn, &QPushButton::pressed, [this, i]() {
            onJogButtonPressed("left", i, 1);
        });
        connect(plusBtn, &QPushButton::released, 
                this, &RobotControlPanel::onJogButtonReleased);
        
        layout->addWidget(label);
        layout->addWidget(minusBtn);
        layout->addWidget(plusBtn);
        
        leftLayout->addWidget(jointWidget);
    }
    
    // Create right arm joint controls
    QVBoxLayout* rightLayout = qobject_cast<QVBoxLayout*>(
        ui->scrollAreaWidgetContents_right->layout());
    if (!rightLayout) {
        rightLayout = new QVBoxLayout(ui->scrollAreaWidgetContents_right);
    }
    
    for (int i = 0; i < 7; ++i) {
        QWidget* jointWidget = new QWidget();
        QHBoxLayout* layout = new QHBoxLayout(jointWidget);
        
        QLabel* label = new QLabel(QString("J%1: 0.000").arg(i+1));
        label->setMinimumWidth(100);
        m_rightJointLabels.append(label);
        
        QPushButton* minusBtn = new QPushButton("-");
        minusBtn->setMaximumWidth(40);
        m_rightJogMinusButtons.append(minusBtn);
        connect(minusBtn, &QPushButton::pressed, [this, i]() {
            onJogButtonPressed("right", i, -1);
        });
        connect(minusBtn, &QPushButton::released, 
                this, &RobotControlPanel::onJogButtonReleased);
        
        QPushButton* plusBtn = new QPushButton("+");
        plusBtn->setMaximumWidth(40);
        m_rightJogPlusButtons.append(plusBtn);
        connect(plusBtn, &QPushButton::pressed, [this, i]() {
            onJogButtonPressed("right", i, 1);
        });
        connect(plusBtn, &QPushButton::released, 
                this, &RobotControlPanel::onJogButtonReleased);
        
        layout->addWidget(label);
        layout->addWidget(minusBtn);
        layout->addWidget(plusBtn);
        
        rightLayout->addWidget(jointWidget);
    }
}

void RobotControlPanel::onConnectClicked()
{
    QString host = ui->lineEditHost->text();
    quint16 port = ui->spinBoxPort->value();
    
    if (m_connection->isConnected()) {
        m_connection->disconnect();
        ui->btnConnect->setText("连接");
    } else {
        m_connection->connectToRobot(host, port);
        ui->btnConnect->setText("连接中...");
    }
}

void RobotControlPanel::onConnected()
{
    qDebug() << "=== RobotControlPanel::onConnected ===";
    
    ui->btnConnect->setText("✓ 已连接 (点击断开)");
    ui->btnConnect->setStyleSheet(
        "QPushButton {"
        "    background-color: #6EE7B7;"
        "    border: none;"
        "    color: #0A0D12;"
        "    font-weight: 600;"
        "}"
        "QPushButton:hover {"
        "    background-color: #7EEFC3;"
        "}"
    );
    
    ui->labelStatus->setText("● 已连接");
    ui->labelStatus->setStyleSheet(
        "QLabel {"
        "    color: #6EE7B7;"
        "    background-color: #1E2636;"
        "    border-radius: 4px;"
        "    padding: 6px;"
        "    font-size: 9pt;"
        "    font-weight: bold;"
        "}"
    );
    
    ui->lineEditHost->setEnabled(false);
    ui->spinBoxPort->setEnabled(false);
    
    ui->btnToggleLeft->setEnabled(true);
    ui->btnToggleRight->setEnabled(true);
    ui->btnGetCurrentAnglesLeft->setEnabled(true);
    ui->btnGetCurrentAnglesRight->setEnabled(true);
    
    // List robots
    m_connection->listRobots();
    
    qDebug() << "UI updated: Connected state";
}

void RobotControlPanel::onDisconnected()
{
    qDebug() << "=== RobotControlPanel::onDisconnected ===";
    
    ui->btnConnect->setText("连接机器人");
    ui->btnConnect->setStyleSheet("");  // 恢复默认样式
    emit robotAvailabilityChanged(false);
    
    ui->labelStatus->setText("● 未连接");
    ui->labelStatus->setStyleSheet(
        "QLabel {"
        "    color: #C8D2E0;"
        "    background-color: #1E2636;"
        "    border-radius: 4px;"
        "    padding: 6px;"
        "    font-size: 9pt;"
        "}"
    );
    
    ui->lineEditHost->setEnabled(true);
    ui->spinBoxPort->setEnabled(true);
    
    ui->btnToggleLeft->setEnabled(false);
    ui->btnToggleLeft->setChecked(false);
    ui->btnToggleRight->setEnabled(false);
    ui->btnToggleRight->setChecked(false);
    ui->btnGetCurrentAnglesLeft->setEnabled(false);
    ui->btnGetCurrentAnglesRight->setEnabled(false);

    // 机械臂断开时灵巧手的 rs485 通道也走不了，本地状态一并作废
    if (m_hand) {
        m_hand->disconnectHands();
    }
    ui->labelHandStatus->setText(QStringLiteral("● 未连接"));
    updateHandUi();
    
    qDebug() << "UI updated: Disconnected state";
}

void RobotControlPanel::onRobotListReceived(const QJsonArray& robots)
{
    qDebug() << "========== onRobotListReceived ==========";
    qDebug() << "Robots received:" << robots;
    
    m_leftArmName.clear();
    m_rightArmName.clear();
    
    // 逐个按名称归类，不再假设"第 0 个就是左臂"
    QStringList unnamed;
    for (const QJsonValue& v : robots) {
        const QJsonObject robot = v.toObject();
        const QString name = robot.value("name").toString();
        if (name.isEmpty()) continue;
        
        const QString arm = armOf(name);
        if (arm == "left" && m_leftArmName.isEmpty()) {
            m_leftArmName = name;
        } else if (arm == "right" && m_rightArmName.isEmpty()) {
            m_rightArmName = name;
        } else {
            unnamed.append(name);
        }
    }
    
    // 名称判断不出左右的，按剩余顺序补齐
    for (const QString& name : unnamed) {
        if (m_leftArmName.isEmpty()) {
            m_leftArmName = name;
        } else if (m_rightArmName.isEmpty()) {
            m_rightArmName = name;
        }
    }
    
    qDebug() << "✓ Left arm name:" << m_leftArmName;
    qDebug() << "✓ Right arm name:" << m_rightArmName;
    
    // 按真实臂名订阅；每条臂一个独立主题
    if (!m_leftArmName.isEmpty()) {
        m_connection->subscribeToState(m_leftArmName, 50);
        m_connection->getRobotStatus(m_leftArmName);
    }
    if (!m_rightArmName.isEmpty()) {
        m_connection->subscribeToState(m_rightArmName, 50);
        m_connection->getRobotStatus(m_rightArmName);
    }
    
    if (m_leftArmName.isEmpty() && m_rightArmName.isEmpty()) {
        qWarning() << "✗ No robots found in list!";
    }

    emit armNamesChanged(m_leftArmName, m_rightArmName);
    emit robotAvailabilityChanged(!m_leftArmName.isEmpty() || !m_rightArmName.isEmpty());
}

QString RobotControlPanel::armOf(const QString& robotName) const
{
    if (robotName.isEmpty()) {
        return QString();
    }
    
    // 优先用 robot/list 得到的真实名做精确匹配
    if (!m_leftArmName.isEmpty() && robotName == m_leftArmName) {
        return "left";
    }
    if (!m_rightArmName.isEmpty() && robotName == m_rightArmName) {
        return "right";
    }
    
    // 再按名称关键字判断
    const QString lower = robotName.toLower();
    if (lower.contains("left") || lower.contains("_l") || lower.endsWith("l")) {
        return "left";
    }
    if (lower.contains("right") || lower.contains("_r") || lower.endsWith("r")) {
        return "right";
    }
    
    return QString();
}

void RobotControlPanel::onRobotStateUpdated(const QJsonObject& state)
{
    // state 结构：{ "name": "<臂名>", "joints": [...], "cartesian": {...}, "enabled": bool, ... }
    const QString robotName = state.value("name").toString();
    
    // 拿不到臂名就无法可靠区分左右，直接丢弃这一帧，
    // 绝不能像以前那样默认算作左臂——那会把右臂数据覆盖到左臂上。
    const QString arm = armOf(robotName);
    if (arm.isEmpty()) {
        qWarning() << "丢弃状态帧：无法判定所属臂，name =" << robotName;
        return;
    }
    
    const bool isLeft = (arm == "left");
    
    // ---- 关节角 ----
    if (state.contains("joints")) {
        const QJsonArray jointsArray = state["joints"].toArray();
        QVector<double> joints;
        joints.reserve(jointsArray.size());
        for (const QJsonValue& val : jointsArray) {
            joints.append(val.toDouble());
        }
        
        if (joints.size() >= 7) {
            if (isLeft) {
                m_leftJoints = joints;
                updateJointDisplay("left", joints);
            } else {
                m_rightJoints = joints;
                updateJointDisplay("right", joints);
            }
            emit jointAnglesChanged(arm, joints);
            qDebug().nospace() << robotName << " [" << arm << "] J1-J3 = "
                               << joints.mid(0, qMin(3, joints.size()));
        }
    }
    
    // ---- TCP 位姿 ----
    if (state.contains("cartesian")) {
        const QJsonObject cart = state["cartesian"].toObject();
        // 按 API 文档：字段为 x / y / z（不是数组 pos）
        const bool hasXYZ = cart.contains("x") || cart.contains("y") || cart.contains("z");
        if (hasXYZ) {
            updateTCPDisplay(arm, cart["x"].toDouble(), cart["y"].toDouble(), cart["z"].toDouble());
        } else if (cart.contains("pos")) {
            const QJsonArray pos = cart["pos"].toArray();
            if (pos.size() >= 3) {
                updateTCPDisplay(arm, pos[0].toDouble(), pos[1].toDouble(), pos[2].toDouble());
            }
        }
    }
    
    // ---- 使能状态 ----
    if (state.contains("enabled")) {
        const bool enabled = state["enabled"].toBool();
        QPushButton* btn = isLeft ? ui->btnToggleLeft : ui->btnToggleRight;
        btn->blockSignals(true);
        btn->setChecked(enabled);
        btn->setText(enabled ? "失能" : "使能");
        btn->blockSignals(false);
    }
}

void RobotControlPanel::onResponseReceived(const QJsonObject& response)
{
    if (!response.contains("err")) {
        return;
    }

    qWarning() << "Error:" << response["err"];

    const QJsonObject err = response["err"].toObject();
    const int code = err.value("code").toInt();
    const QString message = err.value("message").toString();

    // 只处理 jogStart 这一次请求的拒绝：按 id 精确匹配。
    // 点动被拒时界面表现就是「长按没反应」，不提示的话根本无从判断
    // 是没连上、没使能，还是控制模式不对。
    const int id = response.value("id").toInt(-1);
    if (m_jogRequestId >= 0 && id == m_jogRequestId) {
        m_jogRequestId = -1;

        // 20319 = 该臂已有活跃点动会话。典型场景是刚松手又立刻按下：
        // 上一条点动还在平滑减速，会话没关。这不是错误，等减速完再试即可。
        // m_activeJogArm 非空说明用户还按着按钮，此时值得重试；已松手就不必了。
        if (code == 20319 && !m_activeJogArm.isEmpty() && m_jogRetries < kJogMaxRetries) {
            ++m_jogRetries;
            m_jogRetryTimer->start(kJogRetryDelayMs);
            return;
        }

        // 会话没建立，继续发心跳只会不断报错
        m_jogHeartbeatTimer->stop();
        // 清掉活动会话：否则松手时会对着一个不存在的会话发 jogStop。
        // 按钮的视觉按下状态由 Qt 在 released 时自行复位，不需要手动处理。
        m_activeJogArm.clear();

        const QString text = message.isEmpty()
            ? QStringLiteral("点动被拒绝（错误码 %1）").arg(code)
            : QStringLiteral("点动被拒绝：%1").arg(message);
        MessageCenter::warning(this, text);
        return;
    }

    // 点动过程中其它请求被拒（心跳、stop）也值得提示，但不必打断用户
    if (!m_activeJogArm.isEmpty()) {
        qWarning() << "[点动] 请求被拒，code =" << code << "msg =" << message;
    }
}

void RobotControlPanel::updateJointDisplay(const QString& arm, 
                                          const QVector<double>& joints)
{
    QVector<QLabel*>* labels = (arm == "left") ? &m_leftJointLabels : &m_rightJointLabels;
    
    for (int i = 0; i < qMin(joints.size(), labels->size()); ++i) {
        (*labels)[i]->setText(QString("J%1: %2").arg(i+1).arg(joints[i], 0, 'f', 3));
    }
    
    // 不在这里emit，在onRobotStateUpdated中emit
}

void RobotControlPanel::updateTCPDisplay(const QString& arm, double x, double y, double z)
{
    // TCP显示在简化UI中移除了
    Q_UNUSED(arm);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(z);
}

void RobotControlPanel::onToggleLeftClicked(bool checked)
{
    qDebug() << "========== onToggleLeftClicked ==========";
    qDebug() << "m_leftArmName:" << m_leftArmName;
    qDebug() << "checked (target state):" << checked;
    
    if (!m_leftArmName.isEmpty()) {
        if (checked) {
            // 切换到选中状态 = 使能
            qDebug() << "Sending enable command to:" << m_leftArmName;
            m_connection->enableRobot(m_leftArmName);
        } else {
            // 切换到未选中状态 = 失能
            qDebug() << "Sending disable command to:" << m_leftArmName;
            m_connection->disableRobot(m_leftArmName);
        }
    } else {
        qWarning() << "✗ Left arm name is empty!";
        // 恢复按钮状态
        ui->btnToggleLeft->setChecked(!checked);
    }
}

void RobotControlPanel::onToggleRightClicked(bool checked)
{
    qDebug() << "========== onToggleRightClicked ==========";
    qDebug() << "m_rightArmName:" << m_rightArmName;
    qDebug() << "checked (target state):" << checked;
    
    if (!m_rightArmName.isEmpty()) {
        if (checked) {
            // 切换到选中状态 = 使能
            qDebug() << "Sending enable command to:" << m_rightArmName;
            m_connection->enableRobot(m_rightArmName);
        } else {
            // 切换到未选中状态 = 失能
            qDebug() << "Sending disable command to:" << m_rightArmName;
            m_connection->disableRobot(m_rightArmName);
        }
    } else {
        qWarning() << "✗ Right arm name is empty!";
        // 恢复按钮状态
        ui->btnToggleRight->setChecked(!checked);
    }
}

void RobotControlPanel::onGetCurrentAnglesLeftClicked()
{
    // 取当前关节角度：优先用已订阅到的实测值，否则主动拉一帧
    if (!m_leftArmName.isEmpty()) {
        m_connection->getRobotStatus(m_leftArmName);
    }
    emit currentAnglesRequested(m_leftJoints, m_rightJoints);
}

void RobotControlPanel::onGetCurrentAnglesRightClicked()
{
    if (!m_rightArmName.isEmpty()) {
        m_connection->getRobotStatus(m_rightArmName);
    }
    emit currentAnglesRequested(m_leftJoints, m_rightJoints);
}

void RobotControlPanel::onJogButtonPressed(const QString& arm, int joint, int direction)
{
    QString armName = (arm == "left") ? m_leftArmName : m_rightArmName;
    if (armName.isEmpty()) {
        qWarning() << "[点动] 臂名为空，无法点动。arm =" << arm;
        return;
    }

    // 同一时刻控制器只允许一条连续点动会话。上一轮还没停干净时再发 jogStart
    // 会被拒（20319），所以先把手上的会话收掉，再开新的。
    if (!m_activeJogArm.isEmpty() && m_activeJogArm != armName) {
        m_jogHeartbeatTimer->stop();
        m_connection->jogStop(m_activeJogArm);
    }

    m_activeJogArm = armName;
    m_jogJoint = joint;
    m_jogDirection = direction;
    m_jogRetries = 0;
    sendJogStart();
}

void RobotControlPanel::sendJogStart()
{
    if (m_activeJogArm.isEmpty()) {
        return;
    }

    // speed / maxTarget 传 0：由控制器按该机型的安全缺省档与可达最远点决定。
    // timeoutMs 取 500（控制器缺省），配合 200ms 心跳留出一倍余量。
    m_jogRequestId = m_connection->jogStart(
        m_activeJogArm, "joint", QString::number(m_jogJoint), m_jogDirection,
        0.0, 0.0, kJogTimeoutMs);

    // 心跳周期必须明显小于 timeoutMs，否则网络抖动一波就判定超时自动停。
    // 取 timeoutMs 的一半以内是协议建议的做法。
    m_jogHeartbeatTimer->start(kJogHeartbeatMs);
}

void RobotControlPanel::onJogButtonReleased()
{
    // 松开：先停心跳再发 jogStop。
    // 顺序不能反——先 jogStop 再停心跳的话，这中间的心跳仍在续命，
    // 会与 stop 抢同一会话。
    m_jogHeartbeatTimer->stop();
    m_jogRetryTimer->stop();
    if (!m_activeJogArm.isEmpty()) {
        m_connection->jogStop(m_activeJogArm);
        m_activeJogArm.clear();
    }
    m_jogRequestId = -1;
    m_jogRetries = 0;
}

// ============================================================================
// 灵巧手
// ============================================================================

void RobotControlPanel::createHandControls()
{
    // 手位滑块已随 scrollAreaHand 一起移除：灵巧手的实际手位由时间轴里
    // 的手片段编辑（HandSegmentEditDialog）负责，这里保留预设手势作为
    // 快速试手的入口即可，不必再维护一套并行的滑块 UI。
    ui->comboHandPreset->addItem(QStringLiteral("选择预设手势"), QString());
    for (const QString &name : HandProtocol::presetNames()) {
        ui->comboHandPreset->addItem(name, name);
    }
}

void RobotControlPanel::updateHandUi()
{
    // 连接成功即代表要控制手，不再有独立的「启用」开关
    const bool connected = m_hand && m_hand->isReady();

    ui->btnConnectHand->setText(connected ? QStringLiteral("断开灵巧手")
                                          : QStringLiteral("连接灵巧手"));
    ui->btnRefreshHand->setEnabled(connected);
    ui->btnApplyHandPreset->setEnabled(connected);
    ui->comboHandPreset->setEnabled(connected);
}

void RobotControlPanel::onConnectHandClicked()
{
    if (!m_hand) {
        return;
    }

    if (m_hand->isReady()) {
        // 断开：只清本地状态，不去 reset 控制器的串口
        m_hand->disconnectHands();
        ui->labelHandStatus->setText(QStringLiteral("● 未连接"));
        ui->labelHandStatus->setStyleSheet(
            QStringLiteral("QLabel { color:#C8D2E0; background-color:#1E2636;"
                           " border-radius:4px; padding:6px; font-size:9pt; }"));
        updateHandUi();
        return;
    }

    if (!m_connection->isConnected()) {
        ui->labelHandStatus->setText(QStringLiteral("● 请先连接机械臂"));
        return;
    }

    ui->btnConnectHand->setEnabled(false);
    m_hand->connectHands();
    ui->btnConnectHand->setEnabled(true);
    updateHandUi();
}

void RobotControlPanel::onRefreshHandClicked()
{
    if (m_hand) {
        m_hand->refreshPositions();
    }
}

void RobotControlPanel::onApplyHandPresetClicked()
{
    const QString name = ui->comboHandPreset->currentData().toString();
    if (name.isEmpty() || !m_hand) {
        return;
    }
    const QVector<int> pos = HandProtocol::preset(name);
    if (pos.size() != HandProtocol::kDofCount) {
        return;
    }
    m_hand->sendBoth(pos, pos);
}
