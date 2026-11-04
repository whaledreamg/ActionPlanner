#ifndef ROBOT_STATE_MANAGER_H
#define ROBOT_STATE_MANAGER_H

#include <QObject>
#include <QVector>
#include <QMap>

struct RobotState {
    QString name;
    QVector<double> joints;
    double tcpX, tcpY, tcpZ;
    double tcpRx, tcpRy, tcpRz;
    bool enabled;
};

class RobotStateManager : public QObject
{
    Q_OBJECT
public:
    explicit RobotStateManager(QObject *parent = nullptr);
    
    void updateState(const QString& name, const RobotState& state);
    RobotState getState(const QString& name) const;
    
private:
    QMap<QString, RobotState> m_states;
};

#endif
