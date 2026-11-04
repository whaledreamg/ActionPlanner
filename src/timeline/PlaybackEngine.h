#ifndef PLAYBACK_ENGINE_H
#define PLAYBACK_ENGINE_H

#include <QObject>
#include <QVector>
#include <QElapsedTimer>

#include "timeline/TimelineModel.h"

class QTimer;

/**
 * 轨迹播放引擎。
 *
 * 只负责"时间推进 + 插值"，不关心机器人是否连接：
 * 每帧发出 frameReady(time, state)，由外层决定是推给 3D 预览还是发给机器人。
 *
 * 全程异步（QTimer 驱动），不会阻塞 UI 线程。
 */
class PlaybackEngine : public QObject
{
    Q_OBJECT

public:
    explicit PlaybackEngine(QObject *parent = nullptr);

    void setModel(const TimelineModel *model);

    bool isPlaying() const { return m_state == Playing; }
    bool isPaused()  const { return m_state == Paused; }
    bool isActive()  const { return m_state != Idle; }

    double currentTime() const;
    double totalDuration() const;

    /** 在时间轴上求某个时刻的关节角（也用于拖动游标时的预览） */
    JointState evaluateAt(double time) const;

    /**
     * 第一段起点：播放期间用 play() 那一刻冻结的快照，
     * 其余时候用模型的实时姿态。
     *
     * 必须冻结——播放中机械臂在动，实测角每帧都在变，
     * 若插值基准跟着变，目标点就成了一个一直后退的靶子。
     */
    JointState originPose() const;

public slots:
    void play();
    void pause();
    void resume();
    void stop();

    /**
     * 只发出时间轴起点那一帧，不推进时间。
     *
     * 真机入控时 RealtimeController 要先拿到轨迹起点，才能用约 700ms
     * 把机器人从实测角平滑过渡过去；过渡期间它收到的帧都会被丢弃。
     * 所以配合用法是：
     *   1. 入控成功 → primeStartFrame()，把起点交给它做对齐；
     *   2. 收到 leadInFinished → play()，这时时间轴才开始走。
     *
     * 若在入控成功时就 play()，过渡结束时轨迹已跑出 0.7s，
     * 机器人会一步跳到那里，这就是真机上看到的跳变。
     */
    void primeStartFrame();

    /** 暂停状态下跳转到指定时刻并立刻发出该帧 */
    void seek(double time);

    /** 播放到末尾后是否回到起点（默认 true） */
    void setRewindOnFinish(bool on) { m_rewindOnFinish = on; }

    /**
     * 循环播放：播到末尾自动回绕继续，直到显式 stop()。
     *
     * 回绕时不会重新冻结起点——下一圈必须和上一圈走同一条轨迹，
     * 否则每圈起点都是「那一刻的实测姿态」，轨迹会一圈一个样。
     */
    void setLoop(bool on) { m_loop = on; }
    bool loop() const { return m_loop; }

signals:
    void started();
    void paused();
    void resumed();
    void stopped();
    void finished();
    /** 循环模式下每回绕一圈发一次，供界面计数 */
    void looped();

    /** 每帧目标关节角。同线程直连，传值即可。 */
    void frameReady(double time, const JointState &state);
    void progressChanged(double time, double total);

    /**
     * 播放头进入了一个新的片段（索引从 0 开始，-1 表示离开所有片段）。
     *
     * 用于灵巧手这类「每段只做一次」的动作：插值的是机械臂关节角，
     * 灵巧手只在段开始时下发一次目标，之后整段不再重复发。
     */
    void segmentEntered(int index);

private slots:
    void onTick();

    /**
     * 片段改动后重建 spline 关键帧。
     *
     * 不重建的话，编辑完片段直接拖动游标预览，算出来的还是改动前的轨迹。
     */
    void refreshKeyframes();

private:
    enum State { Idle, Playing, Paused };

    void emitFrameAt(double time);
    void buildKeyframes();

    /**
     * 机械臂片段按时间轴起点升序排好的索引。
     *
     * 片段可以被拖到任意位置，列表顺序并不等于时间顺序。
     * 求值和建关键帧都必须按时间顺序走，否则「上一段的终点」
     * 会取成列表里的前一段——拖动过后两者不是同一个片段，轨迹就会跳。
     */
    QVector<int> armIndicesByTime() const;

    /**
     * 第 index 段（时间序）的起点，即时间上位于它之前那个机械臂片段的终点；
     * 前面没有机械臂片段时用 originPose()（播放中即冻结快照）。
     *
     * 不能直接用 TimelineModel::segmentStartJoints()：它在首段返回
     * m_originPose，而那个值由实测角实时更新。播放中手臂在动，
     * 实测角每 50ms 变一次，首段的插值基准就会跟着漂——真机连接时
     * 表现为整条轨迹不断跳变。
     */
    JointState armSegmentStart(const QVector<int> &order, int orderPos) const;

    /** 当前时刻落在哪个片段，无片段返回 -1 */
    int segmentIndexAt(double time) const;
    /** 与已发出的段号比较，变化时发 segmentEntered */
    void updateSegmentTracking(double time);

    struct Keyframe {
        double time = 0.0;
        JointState state;
    };

    const TimelineModel *m_model = nullptr;
    QTimer *m_timer = nullptr;
    QElapsedTimer m_clock;

    State m_state = Idle;
    double m_accumulatedMs = 0.0;   // 已播放的毫秒数（1 倍速，即真实时间）
    double m_lastEmittedTime = -1.0;
    bool m_rewindOnFinish = true;
    bool m_loop = false;

    /** 样条插值用的全局关键帧（仅 spline 片段时才构建） */
    QVector<Keyframe> m_keys;

    /** play() 时冻结的第一段起点，以及是否已冻结 */
    JointState m_originSnapshot;
    bool m_originFrozen = false;

    /**
     * primeStartFrame() 是否已经把起点对齐好。
     *
     * 置位后 play() 沿用同一份快照，不再覆盖。真机入控时机器人
     * 已经按这份快照被带到轨迹起点，若 play() 再取一次实测角，
     * 整条轨迹会平移，交接处出现跳变。
     */
    bool m_primed = false;

    /** 已发出 segmentEntered 的片段索引，用于只在跨越边界时发一次 */
    int m_currentSegment = -1;
};

#endif // PLAYBACK_ENGINE_H
