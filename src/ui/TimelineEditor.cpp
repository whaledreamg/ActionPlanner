#include "ui/TimelineEditor.h"
#include "ui_TimelineEditor.h"
#include "ui/TimelineCanvas.h"
#include "ui/SegmentEditDialog.h"
#include "ui/ArmSegmentEditDialog.h"
#include "ui/HandSegmentEditDialog.h"
#include "ui/TimelineSettingsDialog.h"
#include "ui/MessageCenter.h"
#include "ui/FlowLayout.h"
#include "timeline/PlaybackEngine.h"
#include "robot/RealtimeController.h"
#include "robot/HandController.h"
#include "utils/FileIO.h"

#include <QFileDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QDir>
#include <QLabel>
#include <QPushButton>
#include <QScrollBar>
#include <QSettings>
#include <QDebug>

#include <memory>

TimelineEditor::TimelineEditor(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TimelineEditor)
    , m_canvas(nullptr)
    , m_model(new TimelineModel(this))
    , m_engine(new PlaybackEngine(this))
    , m_realtime(nullptr)
{
    ui->setupUi(this);

    // 工具条改成自动折行布局：横屏平板上宽度也就 1280 左右，
    // 十几个按钮用 QHBoxLayout 一行排不下，会把整个窗口的最小宽度
    // 撑到 1280+ 并超出屏幕，右侧按钮（设置/加载）就被裁掉了。
    if (ui->toolbarContainer) {
        auto *flow = new FlowLayout(ui->toolbarContainer, 0, 8, 8);
        // 按 .ui 里的声明顺序重新挂进 FlowLayout
        const QList<QWidget *> toolbarWidgets = {
            ui->labelTitle,
            ui->btnPlay, ui->btnPause, ui->btnStop,
            ui->checkLoop,
            ui->labelProgress,
            ui->btnAddSegment, ui->btnAddHandSegment,
            ui->btnDuplicateSegment, ui->btnDeleteSegment,
            ui->labelSeparator,
            ui->btnSave, ui->btnLoad, ui->btnSettings,
        };
        for (QWidget *w : toolbarWidgets) {
            if (w) {
                flow->addWidget(w);
            }
        }
    }

    // 画布放进 .ui 预留的容器里；容器本身不再画任何东西，否则会盖住画布
    m_canvas = new TimelineCanvas(ui->timelineCanvas);
    ui->timelineCanvas->setStyleSheet(QString());

    auto *layout = new QVBoxLayout(ui->timelineCanvas);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_canvas);

    // 横向滚动条：时间轴可以铺得比视口长很多（默认 3 分钟），
    // 播放游标或拖动片段把它推到视野外时，用它把视野挪过去。
    m_scrollBar = new QScrollBar(Qt::Horizontal, ui->timelineCanvas);
    m_scrollBar->setStyleSheet(QStringLiteral(
        "QScrollBar:horizontal{background:#161D2B;height:12px;margin:0;border:0;}"
        "QScrollBar::handle:horizontal{background:#33415A;min-width:40px;"
        "border-radius:6px;margin:2px;}"
        "QScrollBar::handle:horizontal:hover{background:#38BDF8;}"
        "QScrollBar::add-line:horizontal,QScrollBar::sub-line:horizontal{width:0;}"
        "QScrollBar::add-page:horizontal,QScrollBar::sub-page:horizontal{"
        "background:#161D2B;}"));
    layout->addWidget(m_scrollBar);

    m_canvas->setModel(m_model);

    m_engine->setModel(m_model);

    connect(ui->btnAddSegment, &QPushButton::clicked, this, &TimelineEditor::onAddSegment);
    connect(ui->btnAddHandSegment, &QPushButton::clicked,
            this, &TimelineEditor::onAddHandSegment);
    connect(ui->btnDeleteSegment, &QPushButton::clicked, this, &TimelineEditor::onDeleteSegment);
    connect(ui->btnDuplicateSegment, &QPushButton::clicked,
            this, &TimelineEditor::onDuplicateSegment);
    connect(ui->btnPlay, &QPushButton::clicked, this, &TimelineEditor::onPlay);
    connect(ui->btnPause, &QPushButton::clicked, this, &TimelineEditor::onPause);
    connect(ui->btnStop, &QPushButton::clicked, this, &TimelineEditor::onStop);
    connect(ui->btnSave, &QPushButton::clicked, this, &TimelineEditor::onSave);
    connect(ui->btnLoad, &QPushButton::clicked, this, &TimelineEditor::onLoad);
    connect(ui->btnSettings, &QPushButton::clicked,
            this, &TimelineEditor::openTimelineSettings);

    // ---- 循环 ----
    connect(ui->checkLoop, &QCheckBox::toggled, this, [this](bool on) {
        m_engine->setLoop(on);
        if (on) {
            setStatusText(QStringLiteral("循环已开启"));
        }
    });

    connect(m_canvas, &TimelineCanvas::selectionChanged, this, [this](int index) {
        const bool active = m_engine->isActive();
        ui->btnDeleteSegment->setEnabled(!active && index >= 0);
        ui->btnDuplicateSegment->setEnabled(!active && index >= 0);
    });
    connect(m_canvas, &TimelineCanvas::segmentEditRequested,
            this, &TimelineEditor::editSegment);
    connect(m_canvas, &TimelineCanvas::segmentDeleteRequested, this, [this](int index) {
        if (index < 0 || index >= m_model->segmentCount()) {
            return;
        }
        m_canvas->setSelectedIndex(index);
        onDeleteSegment();
    });
    connect(m_canvas, &TimelineCanvas::playheadSeekRequested,
            this, &TimelineEditor::onSeek);
    connect(m_canvas, &TimelineCanvas::segmentAddRequested, this, [this](int type) {
        addSegmentOfType(SegmentType(type));
    });

    // ---- 横向滚动条 ----
    connect(m_canvas, &TimelineCanvas::viewRangeChanged,
            this, &TimelineEditor::syncScrollBar);
    connect(m_scrollBar, &QScrollBar::valueChanged, this, [this](int value) {
        if (m_syncingScroll) {
            return;
        }
        // 滚动条用「千分之一秒」为单位，整数够精细且不会溢出
        m_canvas->setViewStart(value / 1000.0);
    });

    loadSettings();     // 读回上次的时间轴长度等偏好
    syncScrollBar();

    // ---- 播放引擎 ----
    connect(m_engine, &PlaybackEngine::started,  this, &TimelineEditor::onPlaybackStarted);
    connect(m_engine, &PlaybackEngine::finished, this, &TimelineEditor::onPlaybackFinished);
    connect(m_engine, &PlaybackEngine::stopped,  this, &TimelineEditor::onPlaybackFinished);
    connect(m_engine, &PlaybackEngine::paused,   this, &TimelineEditor::updatePlaybackUi);
    connect(m_engine, &PlaybackEngine::resumed,  this, &TimelineEditor::updatePlaybackUi);
    connect(m_engine, &PlaybackEngine::frameReady, this,
            [this](double time, const JointState &state) {
                Q_UNUSED(time);
                onPlaybackFrame(state);
            });
    connect(m_engine, &PlaybackEngine::progressChanged,
            this, &TimelineEditor::onPlaybackProgress);
    connect(m_engine, &PlaybackEngine::segmentEntered,
            this, &TimelineEditor::onSegmentEntered);

    ui->btnDeleteSegment->setEnabled(false);

    m_currentLeftArm.fill(0.0, 7);
    m_currentRightArm.fill(0.0, 7);

    updatePlaybackUi();
}

TimelineEditor::~TimelineEditor()
{
    delete ui;
}

void TimelineEditor::setRealtimeController(RealtimeController *controller)
{
    if (m_realtime) {
        disconnect(m_realtime, nullptr, this, nullptr);
    }
    m_realtime = controller;

    if (m_realtime) {
        connect(m_realtime, &RealtimeController::errorOccurred,
                this, &TimelineEditor::onRealtimeError);
        connect(m_realtime, &RealtimeController::statusMessage,
                this, &TimelineEditor::onRealtimeStatus);
        connect(m_realtime, &RealtimeController::streamingStopped, this, [this]() {
            m_pendingPlay = false;
            if (m_engine->isActive()) {
                m_engine->stop();
            }
            updatePlaybackUi();
        });
        connect(m_realtime, &RealtimeController::streamingStarted, this, [this]() {
            if (!m_pendingPlay) {
                return;
            }
            // 入控已确认，但还不能推进时间轴：RealtimeController 要用
            // 约 700ms 把机器人从实测角平滑带到轨迹起点，这期间的帧会被丢弃。
            // 先把起点那一帧交给它做对齐（它会 drive 出过渡轨迹），
            // 等 leadInFinished 再真正开始推进时间轴。
            m_engine->primeStartFrame();
            setStatusText(QStringLiteral("正在对齐轨迹起点…"));
            updatePlaybackUi();
        });
        connect(m_realtime, &RealtimeController::leadInFinished, this, [this]() {
            if (!m_pendingPlay) {
                return;
            }
            m_pendingPlay = false;
            // 起点已对齐，现在才真正开始推进时间轴
            m_engine->play();
            emit playRequested();
            updatePlaybackUi();
        });
    }
    updatePlaybackUi();
}

void TimelineEditor::setHandController(HandController *hand)
{
    m_hand = hand;
}

QVector<int> TimelineEditor::currentHandLeft() const
{
    return m_hand ? m_hand->positions(true) : QVector<int>();
}

QVector<int> TimelineEditor::currentHandRight() const
{
    return m_hand ? m_hand->positions(false) : QVector<int>();
}

bool TimelineEditor::isPlaying() const
{
    return m_engine && m_engine->isActive();
}

void TimelineEditor::setCurrentAngles(const QVector<double>& leftArm,
                                      const QVector<double>& rightArm)
{
    m_currentLeftArm = leftArm;
    m_currentRightArm = rightArm;

    // 第一段的起点就是这里：实时姿态一路跟着实测走，
    // 没有实测值（未连机器人）时保持原样，不回退到全零。
    if (leftArm.size() < 7 || rightArm.size() < 7) {
        return;
    }
    JointState pose;
    for (int i = 0; i < 7; ++i) {
        pose.j[i]  = leftArm[i];
        pose.jr[i] = rightArm[i];
    }
    m_model->setOriginPose(pose);

    // 空闲时把轨迹起点画到画布上，让人看到「从哪儿出发」
    if (!m_engine->isActive()) {
        m_canvas->update();
    }
}

void TimelineEditor::onRobotAvailabilityChanged(bool available)
{
    m_robotAvailable = available;
    updatePlaybackUi();
}

// ---------------------------------------------------------------- 播放

void TimelineEditor::onPlay()
{
    if (m_model->segmentCount() == 0) {
        MessageCenter::warning(this, QStringLiteral("没有可播放的片段"));
        return;
    }

    // 已点过播放、正在等入控或起点对齐：忽略重复点击，
    // 否则会重复触发对齐流程，机器人被反复拉回起点
    if (m_pendingPlay) {
        return;
    }

    if (m_engine->isPaused()) {
        m_engine->resume();
        return;
    }
    if (m_engine->isPlaying()) {
        return;
    }

    // 决定是否下发到机器人
    m_sendToRobot = (m_realtime != nullptr) && m_robotAvailable;

    if (m_sendToRobot) {
        if (m_realtime->isStreaming() && m_realtime->isPlaybackActive()) {
            // 会话已建立且起点已对齐，直接播
            m_engine->play();
            emit playRequested();
        } else if (m_realtime->isStreaming()) {
            // 通道在、但对齐还没完成（可能是同一会话连播第二遍）。
            // 重新走一次对齐，避免机器人从当前位置一步跳向轨迹。
            m_pendingPlay = true;
            m_engine->primeStartFrame();
            setStatusText(QStringLiteral("正在对齐轨迹起点…"));
            updatePlaybackUi();
        } else {
            // 入控是异步的（可能要等实测角），等确认后再真正开播，
            // 否则过渡期发出的帧会被丢掉，画面和机器人会不同步。
            m_pendingPlay = true;
            setStatusText(QStringLiteral("正在进入实时控制…"));
            m_realtime->startRealtime();
        }
        return;
    }

    setStatusText(QStringLiteral("未连接机器人 — 仅本地预览"));
    m_engine->play();
    emit playRequested();
}

void TimelineEditor::onPause()
{
    if (m_engine->isPlaying()) {
        m_engine->pause();
    } else if (m_engine->isPaused()) {
        m_engine->resume();
    }
}

void TimelineEditor::onStop()
{
    // 先清标志，避免 engine->stop() 触发 onPlaybackFinished 时重复停控
    const bool wasSending = m_sendToRobot;
    m_sendToRobot = false;
    m_pendingPlay = false;

    m_engine->stop();

    if (wasSending && m_realtime && m_realtime->isStreaming()) {
        m_realtime->stopRealtime();
    }

    m_canvas->setPlayheadTime(-1.0);
    ui->labelProgress->setText(QStringLiteral("0.00 / 0.00 s"));
    updatePlaybackUi();
}

void TimelineEditor::openTimelineSettings()
{
    TimelineSettingsDialog dlg(m_settings, this);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    const TimelineSettingsDialog::Settings next = dlg.result();
    if (next == m_settings) {
        return;
    }

    // 下发频率变化要单独判断：它只影响实时控制，不需要重排时间轴
    const bool freqChanged = (next.streamFrequency != m_settings.streamFrequency);

    m_settings = next;
    m_canvas->setTimelineDuration(m_settings.timelineDuration);
    m_canvas->setAutoScrollToPlayhead(m_settings.autoScrollToPlayhead);
    saveSettings();
    syncScrollBar();

    if (freqChanged) {
        emit streamFrequencyChanged(m_settings.streamFrequency);
    }

    setStatusText(QStringLiteral("时间轴长度已设为 %1 秒，下发频率 %2 Hz")
                      .arg(m_settings.timelineDuration, 0, 'f', 0)
                      .arg(m_settings.streamFrequency));
}

void TimelineEditor::loadSettings()
{
    QSettings s(QStringLiteral("ActionPlanner"), QStringLiteral("Planner"));
    s.beginGroup(QStringLiteral("timeline"));

    m_settings.timelineDuration =
        s.value(QStringLiteral("duration"), m_settings.timelineDuration).toDouble();
    m_settings.defaultArmDuration =
        s.value(QStringLiteral("armDuration"), m_settings.defaultArmDuration).toDouble();
    m_settings.defaultHandDuration =
        s.value(QStringLiteral("handDuration"), m_settings.defaultHandDuration).toDouble();
    m_settings.autoScrollToPlayhead =
        s.value(QStringLiteral("autoScroll"), m_settings.autoScrollToPlayhead).toBool();

    // 下发频率和其它偏好一起存在本机，重启后沿用上次的档位
    m_settings.streamFrequency =
        s.value(QStringLiteral("streamFrequency"), m_settings.streamFrequency).toInt();

    // 加载动作序列的默认目录。空串是合法值（表示由系统对话框决定），
    // 所以直接用默认值为空的 toString()，不要回退到某个固定路径。
    m_settings.loadFolder =
        s.value(QStringLiteral("loadFolder"), m_settings.loadFolder).toString();

    s.endGroup();

    m_canvas->setTimelineDuration(m_settings.timelineDuration);
    m_canvas->setAutoScrollToPlayhead(m_settings.autoScrollToPlayhead);
}

void TimelineEditor::saveSettings() const
{
    QSettings s(QStringLiteral("ActionPlanner"), QStringLiteral("Planner"));
    s.beginGroup(QStringLiteral("timeline"));
    s.setValue(QStringLiteral("duration"), m_settings.timelineDuration);
    s.setValue(QStringLiteral("armDuration"), m_settings.defaultArmDuration);
    s.setValue(QStringLiteral("handDuration"), m_settings.defaultHandDuration);
    s.setValue(QStringLiteral("autoScroll"), m_settings.autoScrollToPlayhead);
    s.setValue(QStringLiteral("streamFrequency"), m_settings.streamFrequency);
    s.setValue(QStringLiteral("loadFolder"), m_settings.loadFolder);
    s.endGroup();
}

void TimelineEditor::syncScrollBar()
{
    if (!m_scrollBar || !m_canvas) {
        return;
    }

    m_syncingScroll = true;

    // 用千分之一秒为单位：3 分钟 = 180000，远在 int 范围内，
    // 又不至于像「整秒」那样在放大后滚不动
    const double total = m_canvas->effectiveDuration();
    const double visible = m_canvas->visibleDuration();
    const double maxStart = m_canvas->maxViewStart();

    const int maximum = qMax(0, qRound(maxStart * 1000.0));
    m_scrollBar->setRange(0, maximum);
    m_scrollBar->setPageStep(qMax(1, qRound(visible * 1000.0)));
    m_scrollBar->setSingleStep(qMax(1, qRound(visible * 1000.0 * 0.1)));
    m_scrollBar->setValue(qBound(0, qRound(m_canvas->viewStart() * 1000.0), maximum));

    // 内容装得下时没必要显示滚动条，省一行高度
    m_scrollBar->setVisible(maximum > 0);

    Q_UNUSED(total);
    m_syncingScroll = false;
}

void TimelineEditor::onPlaybackStarted()
{
    updatePlaybackUi();
    emit playbackStateChanged(true);
}

void TimelineEditor::onPlaybackFinished()
{
    const bool wasSending = m_sendToRobot;
    m_sendToRobot = false;

    if (wasSending && m_realtime && m_realtime->isStreaming()) {
        m_realtime->stopRealtime();
    }

    updatePlaybackUi();
    emit playbackStateChanged(false);
}

void TimelineEditor::onPlaybackFrame(const JointState& state)
{
    QVector<double> left(7), right(7);
    for (int i = 0; i < 7; ++i) {
        left[i] = state.j[i];
        right[i] = state.jr[i];
    }

    // 3D 视图始终跟随
    emit playbackFrame(left, right);

    // 连了机器人就同时下发
    if (m_sendToRobot && m_realtime && m_realtime->isStreaming()) {
        m_realtime->pushFrame(state);
    }
}

void TimelineEditor::onSegmentEntered(int index)
{
    // 只有夹爪片段会下发手位。机械臂片段完全不含手部信息，
    // 两类片段各自独立，互不触发对方的动作。
    if (index < 0 || index >= m_model->segmentCount()) {
        return;
    }

    const Segment seg = m_model->getSegment(index);
    if (!seg.isHand()) {
        return;     // 机械臂片段：不动灵巧手
    }

    // 纯本地预览时不骚扰硬件
    if (!m_sendToRobot) {
        return;
    }
    // 灵巧手连接成功即代表要控制手，没有单独的启用开关
    if (!m_hand || !m_hand->isReady()) {
        return;
    }

    QVector<int> left(HandProtocol::kDofCount);
    QVector<int> right(HandProtocol::kDofCount);
    for (int d = 0; d < HandProtocol::kDofCount; ++d) {
        left[d] = seg.leftHand.dof[d];
        right[d] = seg.rightHand.dof[d];
    }

    m_hand->sendBoth(left, right);
    qInfo() << "[灵巧手] 片段" << index << seg.label << "下发手位";
}

void TimelineEditor::onPlaybackProgress(double time, double total)
{
    m_canvas->setPlayheadTime(time);
    ui->labelProgress->setText(
        QStringLiteral("%1 / %2 s").arg(time, 0, 'f', 2).arg(total, 0, 'f', 2));
}

void TimelineEditor::onSeek(double time)
{
    if (m_engine->isPlaying()) {
        // 播放中拖动游标：直接跳转，继续播
        m_engine->seek(time);
    } else {
        // 未播放：只更新游标和 3D 预览
        m_canvas->setPlayheadTime(time);
        const JointState s = m_engine->evaluateAt(time);
        QVector<double> left(7), right(7);
        for (int i = 0; i < 7; ++i) {
            left[i] = s.j[i];
            right[i] = s.jr[i];
        }
        emit playbackFrame(left, right);
        ui->labelProgress->setText(
            QStringLiteral("%1 / %2 s")
                .arg(time, 0, 'f', 2)
                .arg(m_engine->totalDuration(), 0, 'f', 2));
    }
}

void TimelineEditor::onRealtimeError(const QString& message)
{
    m_pendingPlay = false;
    setStatusText(message);

    if (m_engine->isActive()) {
        m_engine->stop();
    }
    m_sendToRobot = false;
    updatePlaybackUi();

    // 不用模态 QMessageBox：Android 上按钮收不到触摸事件，会卡住整个界面
    MessageCenter::warning(this, QStringLiteral("实时控制：%1").arg(message));
}
void TimelineEditor::onRealtimeStatus(const QString& message)
{
    setStatusText(message);
}

// ---------------------------------------------------------------- 片段编辑

void TimelineEditor::onAddSegment()
{
    addSegmentOfType(SegmentType::Arm);
}

void TimelineEditor::onAddHandSegment()
{
    addSegmentOfType(SegmentType::Hand);
}

void TimelineEditor::addSegmentOfType(SegmentType type)
{
    if (m_engine->isActive()) {
        MessageCenter::warning(this, QStringLiteral("播放中不能修改片段，请先停止播放"));
        return;
    }

    const bool isHand = (type == SegmentType::Hand);
    const QString typeName = isHand ? QStringLiteral("夹爪片段")
                                    : QStringLiteral("机械臂片段");

    // 直接使用默认值，不弹窗询问标签和时长。
    // 名称和时间后续都能在「编辑片段」里改，多一次确认反而拖慢编排节奏；
    // 而且这个输入框在 Android 上是独立原生窗口，尺寸小、触摸易错位，
    // 弹出后点不动会直接卡住流程。
    const QString label = QStringLiteral("%1%2")
                              .arg(typeName.left(2))
                              .arg(m_model->segmentCount() + 1);
    const double duration = isHand ? m_settings.defaultHandDuration
                                   : m_settings.defaultArmDuration;

    Segment seg;
    seg.type = type;
    seg.label = label;
    seg.startTime = m_model->totalDuration();
    seg.duration = duration;

    if (isHand) {
        // 夹爪片段：默认取当前手位。没连灵巧手就全零，至少有个确定值。
        const QVector<int> lh = currentHandLeft();
        const QVector<int> rh = currentHandRight();
        for (int d = 0; d < HandPose::kDofCount; ++d) {
            seg.leftHand.dof[d]  = (d < lh.size()) ? lh.at(d) : 0;
            seg.rightHand.dof[d] = (d < rh.size()) ? rh.at(d) : 0;
        }
        // 机械臂部分留空：夹爪片段不驱动机械臂，endJoints 不参与任何计算。
    } else {
        seg.interpolationType = QStringLiteral("cubic");

        // 新片段的终点取「此刻的实测姿态」——示教式录制的自然语义：
        // 把手臂 jog 到位，加一段，这一段就负责把手臂带到那里。
        //
        // 不能取 segmentStartJoints()：那返回的是上一个机械臂片段创建时
        // 冻结的 endJoints。连加几段后，所有片段的目标会是同一个旧值，
        // 播放时手臂走完第一段就不动了。
        if (m_currentLeftArm.size() >= 7 && m_currentRightArm.size() >= 7) {
            for (int j = 0; j < 7; ++j) {
                seg.endJoints.j[j]  = m_currentLeftArm.at(j);
                seg.endJoints.jr[j] = m_currentRightArm.at(j);
            }
        } else {
            // 未连机器人、拿不到实测角：退回「原地保持」，至少保证轨迹连续
            seg.endJoints = m_model->segmentStartJoints(m_model->segmentCount());
        }
    }

    m_model->addSegment(seg);
    m_canvas->setSelectedIndex(m_model->segmentCount() - 1);
    m_canvas->update();
    updatePlaybackUi();     // 循环勾选框在无片段时不可用

    emit segmentAdded(seg);
}

void TimelineEditor::onDeleteSegment()
{
    if (m_engine->isActive()) {
        MessageCenter::warning(this, QStringLiteral("播放中不能修改片段，请先停止播放"));
        return;
    }

    const int index = m_canvas->selectedIndex();
    if (index < 0) {
        MessageCenter::warning(this, QStringLiteral("请先点击选中一个片段"));
        return;
    }
    m_model->removeSegment(index);
    m_canvas->setSelectedIndex(-1);
    m_canvas->update();
    updatePlaybackUi();
}

void TimelineEditor::onDuplicateSegment()
{
    if (m_engine->isActive()) {
        MessageCenter::warning(this, QStringLiteral("播放中不能修改片段，请先停止播放"));
        return;
    }

    const int index = m_canvas->selectedIndex();
    if (index < 0) {
        MessageCenter::warning(this, QStringLiteral("请先点击选中一个片段"));
        return;
    }

    // 深拷贝一份，随后把起点挪到时间轴末尾。
    // 只改 startTime，其余（类型、时长、关节角/手位、插值方式）全部沿用，
    // 所以复制出来的片段和源片段一模一样，适合「同一动作再来一遍」的场景。
    Segment copy = m_model->getSegment(index);
    copy.startTime = m_model->totalDuration();
    // 标签加后缀便于区分，但连续复制多份时不叠加成「X 副本 副本」
    if (!copy.label.endsWith(QStringLiteral(" 副本"))) {
        copy.label = QStringLiteral("%1 副本").arg(copy.label);
    }

    m_model->addSegment(copy);
    m_canvas->setSelectedIndex(m_model->segmentCount() - 1);
    m_canvas->update();
    updatePlaybackUi();

    emit segmentAdded(copy);
}

void TimelineEditor::editSegment(int index)
{
    if (index < 0 || index >= m_model->segmentCount()) {
        return;
    }
    if (m_engine->isActive()) {
        MessageCenter::warning(this, QStringLiteral("播放中不能修改片段，请先停止播放"));
        return;
    }

    // 本段起点 = 上一段终点（前面没有机械臂片段则是当前实测姿态），由模型推导
    const JointState derivedStart = m_model->segmentStartJoints(index);
    const Segment seg = m_model->getSegment(index);
    const bool editingHand = seg.isHand();

    // 机械臂片段与夹爪片段用两个独立窗口：Android 上页签头切换后不重绘，
    // 拆开后各窗口只显示自己相关的内容，也就不存在页签刷新问题。
    std::unique_ptr<SegmentEditDialog> dlg;
    if (editingHand) {
        dlg = std::make_unique<HandSegmentEditDialog>(seg, derivedStart, this);
    } else {
        dlg = std::make_unique<ArmSegmentEditDialog>(seg, derivedStart, this);
    }

    dlg->setCurrentAngles(m_currentLeftArm, m_currentRightArm);
    dlg->setCurrentHand(currentHandLeft(), currentHandRight(),
                        m_hand && m_hand->isReady());

    QMetaObject::Connection previewConn = connect(
        dlg.get(), &SegmentEditDialog::previewChanged, this,
        [this, editingHand](bool previewEnd, const QVector<double>& left, const QVector<double>& right) {
            Q_UNUSED(previewEnd);
            // 夹爪片段不含关节目标，预览它会把 3D 手臂拉到零位，
            // 看着像手臂要动——直接不预览，让手臂停在原处。
            if (editingHand) {
                return;
            }
            emit previewRequested(left, right);
        });

    const int ret = dlg->exec();
    disconnect(previewConn);

    if (ret != QDialog::Accepted) {
        if (!m_currentLeftArm.isEmpty() || !m_currentRightArm.isEmpty()) {
            emit previewRequested(m_currentLeftArm, m_currentRightArm);
        }
        return;
    }

    m_model->updateSegment(index, dlg->result());
    m_canvas->update();
}

// ---------------------------------------------------------------- 文件

QString TimelineEditor::loadStartFolder() const
{
    // 未设置：返回空串，让系统对话框自己决定（通常沿用上次打开的位置）
    if (m_settings.loadFolder.isEmpty()) {
        return QString();
    }

    // 配置的目录可能已经不在了（U 盘拔掉、网络盘断开、被手动删掉）。
    // 这时不能把不存在的路径塞给对话框——部分平台会直接落在根目录或
    // 上一个奇怪的位置。退回空串，行为与「未设置」一致。
    const QDir dir(m_settings.loadFolder);
    if (!dir.exists()) {
        qWarning() << "[文件] 配置的加载目录不存在，改用系统默认位置:"
                   << m_settings.loadFolder;
        return QString();
    }
    return m_settings.loadFolder;
}

void TimelineEditor::onSave()
{
    if (m_model->segmentCount() == 0) {
        MessageCenter::warning(this, QStringLiteral("还没有片段可保存"));
        return;
    }

    const QString fileName = QFileDialog::getSaveFileName(
        this, QStringLiteral("保存动作序列"),
        QStringLiteral("action.json"), QStringLiteral("JSON Files (*.json)"));
    if (fileName.isEmpty()) {
        return;
    }

    QJsonObject root;
    // 4.0：片段分类型（arm / hand），两类片段各存各的字段
    root["version"] = QStringLiteral("4.0");
    root["totalDuration"] = m_model->totalDuration();

    // 起始姿态不落盘：第一段的起点永远是「播放那一刻的实测姿态」，
    // 存成固定值只会在换一个初始位形时把机器人拽过去。

    QJsonArray segments;
    for (int i = 0; i < m_model->segmentCount(); ++i) {
        const Segment seg = m_model->getSegment(i);

        QJsonObject segObj;
        segObj["type"] = seg.isHand() ? QStringLiteral("hand")
                                      : QStringLiteral("arm");
        segObj["label"] = seg.label;
        segObj["startTime"] = seg.startTime;
        segObj["duration"] = seg.duration;

        if (seg.isHand()) {
            // 夹爪片段只存手位，不写关节角——它不驱动机械臂
            QJsonArray lh, rh;
            for (int d = 0; d < HandPose::kDofCount; ++d) {
                lh.append(seg.leftHand.dof[d]);
                rh.append(seg.rightHand.dof[d]);
            }
            QJsonObject handObj;
            handObj["leftHand"] = lh;
            handObj["rightHand"] = rh;
            segObj["hand"] = handObj;
        } else {
            segObj["interpolation"] = seg.interpolationType;

            QJsonArray leftEnd, rightEnd;
            for (int j = 0; j < 7; ++j) {
                leftEnd.append(seg.endJoints.j[j]);
                rightEnd.append(seg.endJoints.jr[j]);
            }

            QJsonObject endJoints;
            endJoints["leftArm"] = leftEnd;
            endJoints["rightArm"] = rightEnd;
            segObj["endJoints"] = endJoints;
        }

        segments.append(segObj);
    }
    root["segments"] = segments;

    if (FileIO::saveToJSON(fileName, root)) {
        MessageCenter::success(this, QStringLiteral("动作序列已保存"));
    } else {
        MessageCenter::error(this, QStringLiteral("保存失败"));
    }
}

void TimelineEditor::onLoad()
{
    if (m_engine->isActive()) {
        MessageCenter::warning(this, QStringLiteral("播放中不能加载，请先停止播放"));
        return;
    }

    const QString fileName = QFileDialog::getOpenFileName(
        this, QStringLiteral("加载动作序列"),
        loadStartFolder(), QStringLiteral("JSON Files (*.json)"));
    if (fileName.isEmpty()) {
        return;
    }

    const QJsonObject root = FileIO::loadFromJSON(fileName);
    if (root.isEmpty()) {
        MessageCenter::error(this, QStringLiteral("文件读取失败"));
        return;
    }

    while (m_model->segmentCount() > 0) {
        m_model->removeSegment(0);
    }

    // 起始姿态不参与加载：第一段的起点取自当前实测角，
    // 旧文件里的 startPose / startJoints 直接忽略。
    const QJsonArray segments = root["segments"].toArray();
    for (const QJsonValue& val : segments) {
        const QJsonObject segObj = val.toObject();

        Segment seg;
        const QString typeStr = segObj.value("type").toString();
        if (typeStr == QLatin1String("hand")) {
            seg.type = SegmentType::Hand;
        } else if (typeStr == QLatin1String("arm")) {
            seg.type = SegmentType::Arm;
        } else {
            // 老文件（3.0 及以前）没有 type 字段：那时灵巧手只是机械臂片段的
            // 附属项，hasHandAction 为 true 的片段仍然有机械臂轨迹，
            // 所以整体按机械臂片段读入。
            seg.type = SegmentType::Arm;
        }

        seg.label = segObj["label"].toString();
        seg.startTime = segObj["startTime"].toDouble();
        seg.duration = segObj["duration"].toDouble();
        seg.interpolationType = segObj["interpolation"].toString();
        if (seg.interpolationType.isEmpty()) {
            seg.interpolationType = QStringLiteral("cubic");
        }

        if (seg.isArm()) {
            const QJsonObject endJoints = segObj["endJoints"].toObject();
            const QJsonArray leftEnd  = endJoints["leftArm"].toArray();
            const QJsonArray rightEnd = endJoints["rightArm"].toArray();

            for (int j = 0; j < 7; ++j) {
                if (j < leftEnd.size())  seg.endJoints.j[j]  = leftEnd[j].toDouble();
                if (j < rightEnd.size()) seg.endJoints.jr[j] = rightEnd[j].toDouble();
            }
        }

        // 手位：两类片段都读，夹爪片段全靠它
        const QJsonObject handObj = segObj.value("hand").toObject();
        const QJsonArray lh = handObj.value("leftHand").toArray();
        const QJsonArray rh = handObj.value("rightHand").toArray();
        for (int d = 0; d < HandPose::kDofCount; ++d) {
            if (d < lh.size()) seg.leftHand.dof[d]  = qBound(0, lh[d].toInt(0), 1000);
            if (d < rh.size()) seg.rightHand.dof[d] = qBound(0, rh[d].toInt(0), 1000);
        }

        m_model->addSegment(seg);
    }

    m_canvas->setSelectedIndex(-1);
    m_canvas->setPlayheadTime(-1.0);
    m_canvas->update();

    MessageCenter::success(
        this, QStringLiteral("已加载 %1 个片段").arg(m_model->segmentCount()));
}

// ---------------------------------------------------------------- 界面状态

void TimelineEditor::updatePlaybackUi()
{
    const bool playing = m_engine->isPlaying();
    const bool paused  = m_engine->isPaused();
    // 入控/对齐期间引擎还没启动，但仍属「播放流程进行中」：
    // 编辑按钮必须一并锁住，否则能在对齐过程中改片段
    const bool active  = m_engine->isActive() || m_pendingPlay;

    ui->btnPlay->setEnabled(!playing && !m_pendingPlay);
    ui->btnPlay->setText(paused ? QStringLiteral("▶ 继续") : QStringLiteral("▶ 播放"));
    ui->btnPause->setEnabled(playing || paused);
    ui->btnPause->setText(paused ? QStringLiteral("▶ 继续") : QStringLiteral("⏸ 暂停"));
    ui->btnStop->setEnabled(active);
    ui->checkLoop->setEnabled(m_model->segmentCount() > 0);

    // 播放时禁止编辑，避免轨迹在播放中途被改
    const bool canEdit = !active;
    ui->btnAddSegment->setEnabled(canEdit);
    ui->btnAddHandSegment->setEnabled(canEdit);
    ui->btnDeleteSegment->setEnabled(canEdit && m_canvas->selectedIndex() >= 0);
    ui->btnDuplicateSegment->setEnabled(canEdit && m_canvas->selectedIndex() >= 0);
    ui->btnLoad->setEnabled(canEdit);
}

void TimelineEditor::setStatusText(const QString& text)
{
    ui->labelProgress->setToolTip(text);
    qInfo() << "[播放]" << text;
}
