#include "robot/RobotStateManager.h"

RobotStateManager::RobotStateManager(QObject *parent)
    : QObject(parent)
{
}

void RobotStateManager::updateState(const QString& name, const RobotState& state)
{
    m_states[name] = state;
}

RobotState RobotStateManager::getState(const QString& name) const
{
    return m_states.value(name);
}
