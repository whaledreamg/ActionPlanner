#ifndef SIMULATION_WIDGET_H
#define SIMULATION_WIDGET_H

#include <QWidget>
#include <QVector>
#include <QComboBox>

class RobotRenderer;
class URDFLoader;

namespace Ui {
class SimulationWidget;
}

class SimulationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SimulationWidget(QWidget *parent = nullptr);
    ~SimulationWidget();

signals:
    void urdfLoaded(const QString& filePath);

public slots:
    void updateJointAngles(const QString& armName, const QVector<double>& joints);

private slots:
    void onLoadClicked();
    void onBrowseClicked();

private:
    void scanURDFFiles();
    void loadURDF(const QString& filePath);
    
    Ui::SimulationWidget *ui;
    RobotRenderer *m_renderer;
    URDFLoader *m_urdfLoader;
};

#endif
