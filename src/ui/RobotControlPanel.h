#ifndef ROBOTCONTROLPANEL_H
#define ROBOTCONTROLPANEL_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVector>
#include "robot/RobotConnection.h"
#include "robot/HandController.h"

namespace Ui {
class RobotControlPanel;
}

class RobotControlPanel : public QWidget
{
    Q_OBJECT

public:
    explicit RobotControlPanel(QWidget *parent = nullptr);
    ~RobotControlPanel();
    
    QVector<double> getLeftArmJoints() const { return m_leftJoints; }
    QVector<double> getRightArmJoints() const { return m_rightJoints; }

    /** 供实时控制复用同一条 WebSocket 连接 */
    RobotConnection* connection() const { return m_connection; }
    /** 灵巧手控制器（复用同一条连接，但入控流程独立） */
    HandController* handController() const { return m_hand; }

signals:
    void jointAnglesChanged(const QString& armName, const QVector<double>& joints);
    void currentAnglesRequested(const QVector<double>& leftArm, const QVector<double>& rightArm);

    /** 机器人列表/连接状态变化，供播放功能判断能否下发 */
    void robotAvailabilityChanged(bool available);
    /** 左右臂的真实机器人名 */
    void armNamesChanged(const QString& left, const QString& right);

private slots:
    void onConnectClicked();
    void onToggleLeftClicked(bool checked);
    void onToggleRightClicked(bool checked);
    void onGetCurrentAnglesLeftClicked();
    void onGetCurrentAnglesRightClicked();

    // 灵巧手
    void onConnectHandClicked();
    void onRefreshHandClicked();
    void onApplyHandPresetClicked();
    
    void onConnected();
    void onDisconnected();
    void onRobotStateUpdated(const QJsonObject& state);
    void onRobotListReceived(const QJsonArray& robots);
    void onResponseReceived(const QJsonObject& response);
    
    void onJogButtonPressed(const QString& arm, int joint, int direction);
    void onJogButtonReleased();

private:
    void createJointControls();
    void updateJointDisplay(const QString& arm, const QVector<double>& joints);
    void updateTCPDisplay(const QString& arm, double x, double y, double z);
    void createHandControls();
    void updateHandUi();

    /** 根据机器人名判断属于哪个臂。返回 "left"/"right"，无法判定返回空串。
     *  整个类只保留这一处判定逻辑，避免多处判断互相不一致。 */
    QString armOf(const QString& robotName) const;
    
    Ui::RobotControlPanel *ui;
    RobotConnection *m_connection;
    HandController *m_hand = nullptr;

    QString m_leftArmName;
    QString m_rightArmName;
    QVector<double> m_leftJoints;
    QVector<double> m_rightJoints;
    
    // Joint control widgets
    QVector<QLabel*> m_leftJointLabels;
    QVector<QPushButton*> m_leftJogPlusButtons;
    QVector<QPushButton*> m_leftJogMinusButtons;
    
    QVector<QLabel*> m_rightJointLabels;
    QVector<QPushButton*> m_rightJogPlusButtons;
    QVector<QPushButton*> m_rightJogMinusButtons;
    
    QTimer *m_jogHeartbeatTimer;
    QTimer *m_jogRetryTimer = nullptr;
    QString m_activeJogArm;
    /** 在途 jogStart 的请求 id，用于把它的拒绝原因精确匹配出来 */
    int m_jogRequestId = -1;

    /** 当前点动的关节序号与方向：被 20319 拒后要按原参数重发 */
    int m_jogJoint = 0;
    int m_jogDirection = 1;
    int m_jogRetries = 0;

    /** 按当前 m_jogJoint / m_jogDirection 发一次 jogStart 并起心跳 */
    void sendJogStart();
};

#endif // ROBOTCONTROLPANEL_H
