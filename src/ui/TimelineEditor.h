#ifndef TIMELINE_EDITOR_H
#define TIMELINE_EDITOR_H

#include <QWidget>
#include <QVector>
#include "timeline/TimelineModel.h"
#include "ui/TimelineSettingsDialog.h"

class TimelineCanvas;
class PlaybackEngine;
class RealtimeController;
class HandController;
class QScrollBar;

namespace Ui {
class TimelineEditor;
}

/**
 * 时间轴编辑器：工具栏 + 画布 + 播放控制。
 *
 * 职责划分：
 *  - TimelineCanvas  绘制与鼠标交互
 *  - PlaybackEngine  时间推进与插值（不关心机器人）
 *  - RealtimeController  realtime 入控与 UDP 下发
 *
 * 本类只做编排与界面状态同步。
 */
class TimelineEditor : public QWidget
{
    Q_OBJECT

public:
    explicit TimelineEditor(QWidget *parent = nullptr);
    ~TimelineEditor();

    void setRealtimeController(RealtimeController *controller);

    /** 灵巧手。可为空——不看灵巧手时整个手功能都不参与播放。 */
    void setHandController(HandController *hand);

    bool isPlaying() const;

    /** 当前配置的 UDP 下发频率（Hz） */
    int streamFrequency() const { return m_settings.streamFrequency; }

signals:
    void playRequested();
    void segmentAdded(const Segment& segment);

    /** 设置页里改了下发频率（Hz），供主窗口转给 RealtimeController */
    void streamFrequencyChanged(int hz);

    /** 编辑对话框请求 3D 预览某组关节角 */
    void previewRequested(const QVector<double>& leftArm, const QVector<double>& rightArm);

    /** 播放时每帧的关节角，供 3D 视图同步 */
    void playbackFrame(const QVector<double>& leftArm, const QVector<double>& rightArm);

    /** 播放状态变化（用于主窗口更新按钮/提示） */
    void playbackStateChanged(bool active);

public slots:
    void setCurrentAngles(const QVector<double>& leftArm, const QVector<double>& rightArm);

    /** 供主窗口在机器人连接状态变化时调用 */
    void onRobotAvailabilityChanged(bool available);

    /** 供主窗口「文件」菜单调用，复用工具条按钮的同一套逻辑 */
    void onSave();
    void onLoad();

    /** 新建动作序列：清空时间轴，从头开始编排 */
    void onNew();

private slots:
    void onAddSegment();
    /** 添加夹爪片段：只驱动灵巧手，机械臂保持不动 */
    void onAddHandSegment();
    void onDeleteSegment();
    /** 复制选中片段并追加到时间轴末尾 */
    void onDuplicateSegment();
    void onPlay();
    void onPause();
    void onStop();

    void onPlaybackStarted();
    void onPlaybackFinished();
    void onPlaybackFrame(const JointState& state);
    void onPlaybackProgress(double time, double total);
    void onSeek(double time);
    void onRealtimeError(const QString& message);
    void onRealtimeStatus(const QString& message);

    /** 播放头进入新片段：在这里下发该段的灵巧手动作（只发一次） */
    void onSegmentEntered(int index);

private:
    void editSegment(int index);
    void addSegmentOfType(SegmentType type);
    /** 把画布的可滚动范围同步到滚动条 */
    void syncScrollBar();
    /** 打开时间轴设置对话框 */
    void openTimelineSettings();
    /** 从 QSettings 读回上次的时间轴配置 */
    void loadSettings();
    void saveSettings() const;
    void updatePlaybackUi();
    void setStatusText(const QString& text);

    /** 当前手位，供编辑对话框「取当前手位」使用 */
    QVector<int> currentHandLeft() const;
    QVector<int> currentHandRight() const;

    /**
     * 「加载动作序列」文件对话框的起始目录。
     *
     * 返回空串表示不指定，交给系统对话框决定。配置的目录不存在时
     * 也返回空串——把无效路径交给对话框会落到不可预期的位置。
     */
    QString loadStartFolder() const;

    Ui::TimelineEditor *ui;
    TimelineCanvas *m_canvas;
    QScrollBar *m_scrollBar = nullptr;
    TimelineModel *m_model;
    PlaybackEngine *m_engine;
    RealtimeController *m_realtime;
    HandController *m_hand = nullptr;

    QVector<double> m_currentLeftArm;
    QVector<double> m_currentRightArm;

    bool m_robotAvailable = false;
    /** 是否把帧真正下发给机器人（false 仅本地预览） */
    bool m_sendToRobot = false;
    /** 已点播放、正在等入控确认 */
    bool m_pendingPlay = false;

    /** 防止滚动条与画布互相触发形成回环 */
    bool m_syncingScroll = false;

    /** 时间轴偏好设置（长度、默认片段时长、自动跟随、下发频率） */
    TimelineSettingsDialog::Settings m_settings;
};

#endif
