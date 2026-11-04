#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include "RobotControlPanel.h"
#include "SimulationWidget.h"
#include "TimelineEditor.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/**
 * 主窗口。
 *
 * 直接继承 QWidget，而不是 QMainWindow：QMainWindow 会额外引入菜单栏
 * （QMenuBar）、中央部件（QWidget）、状态栏（QStatusBar）三个各自独立的
 * 几何区域，最小尺寸不在它们之间传递。在 Android 横屏平板上实测，窗口会
 * 被内容撑到比屏幕高，而 QMainWindow 自身不做裁剪也不会缩放，于是最底部
 * 的「动作时间轴」整块被挤出屏幕外——表现就是显示不全。
 *
 * 改成单个 QWidget + 一层纵向布局后，左右分栏和底部时间轴都在同一个
 * QVBoxLayout 里按权重分配空间，尺寸传递正常，不会再出现某块被裁掉的情况。
 *
 * 这里不再放 QMenuBar：保存/加载/退出等功能在时间轴工具条上已有按钮，
 * 菜单属于重复入口，而且会占掉平板本就不多的高度。
 */
class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onArmAnglesChanged(const QString& armName, const QVector<double>& joints);
    void onRobotAvailabilityChanged(bool available);
    void showPose(const QVector<double>& left, const QVector<double>& right);

private:
    void showStatus(const QString& message);

    Ui::MainWindow *ui;
    RobotControlPanel *m_controlPanel;
    SimulationWidget *m_simulationWidget;
    TimelineEditor *m_timelineEditor;
    class RealtimeController *m_realtime;

    // 最近一次实测的左右臂关节角
    QVector<double> m_currentLeft;
    QVector<double> m_currentRight;

    /**
     * 时间轴是否正在播放。
     *
     * 播放中 3D 视图的数据源唯一：playbackFrame 送来的计算值。
     * 实测角此时会滞后于下发的指令，两路同时写模型会让画面反复跳变。
     */
    bool m_playbackActive = false;
};

#endif // MAINWINDOW_H
