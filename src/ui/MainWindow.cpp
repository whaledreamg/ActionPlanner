#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "robot/RealtimeController.h"

#include <QSplitter>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("双臂机械臂动作规划器");

    // Create widgets
    m_controlPanel = new RobotControlPanel(this);
    m_simulationWidget = new SimulationWidget(this);
    m_timelineEditor = new TimelineEditor(this);

    // 实时控制需要控制面板里的 RobotConnection 与臂名
    m_realtime = new RealtimeController(m_controlPanel->connection(), this);
    m_timelineEditor->setRealtimeController(m_realtime);

    // 灵巧手：与机械臂实时控制完全独立，未连接时时间轴里的手位不会下发
    m_timelineEditor->setHandController(m_controlPanel->handController());

    // 控制面板和 3D 视图左右并排，时间轴在下面
    ui->controlPanelLayout->addWidget(m_controlPanel);
    ui->simulationLayout->addWidget(m_simulationWidget);
    ui->timelineAreaLayout->addWidget(m_timelineEditor);

    // 上下分栏：上面是控制面板 + 3D 视图，下面是时间轴。
    // 用 QSplitter 而不是固定比例，用户可以按当前任务拖动分配高度；
    // 初始给时间轴一个够用的高度，并把多出来的空间给上半部分。
    ui->mainSplitter->setStretchFactor(0, 3);
    ui->mainSplitter->setStretchFactor(1, 2);

    // ---- 机器人状态 -> 3D 视图与时间轴 ----
    // 3D 视图不在此直接连 jointAnglesChanged：
    // 播放中它有唯一的数据源（playbackFrame 的计算值），
    // 若同时让实测角也来驱动，两个源以不同频率交替写同一组关节角，
    // 模型就会来回抖。改由 onArmAnglesChanged 按播放状态择一。
    connect(m_controlPanel, &RobotControlPanel::jointAnglesChanged,
            this, &MainWindow::onArmAnglesChanged);
    // 实测角同时作为入控首包基准，必须每次都喂给 RealtimeController
    connect(m_controlPanel, &RobotControlPanel::jointAnglesChanged,
            m_realtime, &RealtimeController::setActualJoints);

    connect(m_controlPanel, &RobotControlPanel::currentAnglesRequested,
            m_timelineEditor, &TimelineEditor::setCurrentAngles);

    // ---- 臂名与连接状态 ----
    connect(m_controlPanel, &RobotControlPanel::armNamesChanged,
            m_realtime, &RealtimeController::setArmNames);
    connect(m_controlPanel, &RobotControlPanel::robotAvailabilityChanged,
            this, &MainWindow::onRobotAvailabilityChanged);

    // ---- UDP 下发频率 ----
    // 改在「设置」里配置，和其它本机偏好一起持久化；改动即时生效，
    // 包括正在播放时（下次入控也会用新值）
    connect(m_timelineEditor, &TimelineEditor::streamFrequencyChanged,
            m_realtime, &RealtimeController::setFrequency);
    m_realtime->setFrequency(m_timelineEditor->streamFrequency());

    // ---- 片段编辑时的 3D 预览 ----
    connect(m_timelineEditor, &TimelineEditor::previewRequested,
            this, &MainWindow::showPose);

    // ---- 播放时 3D 视图跟随 ----
    connect(m_timelineEditor, &TimelineEditor::playbackFrame,
            this, &MainWindow::showPose);

    // 播放状态到状态栏，并记录播放中（3D 视图据此决定用哪个数据源）
    connect(m_timelineEditor, &TimelineEditor::playbackStateChanged,
            this, [this](bool active) {
                m_playbackActive = active;
                if (!active && m_currentLeft.size() >= 7 && m_currentRight.size() >= 7) {
                    // 播放结束：把模型交还给实测姿态。
                    // 否则视图会僵在轨迹终点，与真机当前位置对不上。
                    m_simulationWidget->updateJointAngles("left", m_currentLeft);
                    m_simulationWidget->updateJointAngles("right", m_currentRight);
                }
                showStatus(active ? QStringLiteral("正在播放动作序列…")
                                  : QStringLiteral("播放结束"));
            });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showStatus(const QString& message)
{
    if (ui->labelStatusBar) {
        ui->labelStatusBar->setText(message);
    }
}

void MainWindow::onArmAnglesChanged(const QString& armName, const QVector<double>& joints)
{
    if (armName == QLatin1String("left")) {
        m_currentLeft = joints;
    } else if (armName == QLatin1String("right")) {
        m_currentRight = joints;
    } else {
        return;
    }

    // 实测角始终回填为「轨迹起点」的候选值，与 3D 是否跟实测无关
    m_timelineEditor->setCurrentAngles(m_currentLeft, m_currentRight);

    // 播放中 3D 视图只认 playbackFrame 的计算值。
    // 实测角此时是「机器人跟随指令后的结果」，天然滞后于指令；
    // 若两路都在写模型，画面就会在指令值和实测值之间来回跳。
    if (m_playbackActive) {
        return;
    }
    if (joints.size() >= 7) {
        m_simulationWidget->updateJointAngles(armName, joints);
    }
}

void MainWindow::onRobotAvailabilityChanged(bool available)
{
    m_timelineEditor->onRobotAvailabilityChanged(available);
    showStatus(available ? QStringLiteral("机器人已就绪 — 播放将下发实时控制")
                         : QStringLiteral("未连接机器人 — 播放仅在本地预览"));
}

void MainWindow::showPose(const QVector<double>& left, const QVector<double>& right)
{
    m_simulationWidget->updateJointAngles("left", left);
    m_simulationWidget->updateJointAngles("right", right);
}
