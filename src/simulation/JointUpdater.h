#ifndef JOINT_UPDATER_H
#define JOINT_UPDATER_H

#include <QObject>

class JointUpdater : public QObject
{
    Q_OBJECT
public:
    explicit JointUpdater(QObject *parent = nullptr);
};

#endif
