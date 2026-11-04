#include "timeline/PlaybackEngine.h"
#include "timeline/InterpolationEngine.h"

#include <QTimer>
#include <QDebug>

#include <algorithm>

namespace {
/** 预览帧率。60 Hz 足够顺滑，也不会把 UI 线程压满 */
constexpr int kFrameIntervalMs = 16;
}

PlaybackEngine::PlaybackEngine(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    m_timer->setTimerType(Qt::PreciseTimer);
    m_timer->setInterval(kFrameIntervalMs);
    connect(m_timer, &QTimer::timeout, this, &PlaybackEngine::onTick);
}

void PlaybackEngine::setModel(const TimelineModel *model)
{
    if (m_model) {
        disconnect(m_model, nullptr, this, nullptr);
    }
    m_model = model;
    if (m_model) {
        // 片段改动后关键帧必须重建，否则 spline 走的是旧轨迹
        connect(m_model, &TimelineModel::segmentsChanged,
                this, &PlaybackEngine::refreshKeyframes);
    }
    buildKeyframes();
}

void PlaybackEngine::refreshKeyframes()
{
    // 播放中不重建：中途换掉关键帧会让正在走的轨迹跳变。
    // 播放结束后 play() 会重新构建，届时自然生效。
    if (m_state == Playing) {
        return;
    }
    buildKeyframes();
}

double PlaybackEngine::totalDuration() const
{
    return m_model ? m_model->totalDuration() : 0.0;
}

double PlaybackEngine::currentTime() const
{
    return m_accumulatedMs / 1000.0;
}

QVector<int> PlaybackEngine::armIndicesByTime() const
{
    QVector<int> order;
    if (!m_model) {
        return order;
    }
    for (int i = 0; i < m_model->segmentCount(); ++i) {
        if (m_model->getSegment(i).isArm()) {
            order.append(i);
        }
    }
    // 按时间轴位置排序；起始时间相同的保持原有先后，行为可预期
    std::stable_sort(order.begin(), order.end(), [this](int a, int b) {
        return m_model->getSegment(a).startTime < m_model->getSegment(b).startTime;
    });
    return order;
}

JointState PlaybackEngine::armSegmentStart(const QVector<int> &order, int orderPos) const
{
    if (orderPos <= 0) {
        // 时间序上的首个机械臂片段：从出发姿态开始
        return originPose();
    }
    return m_model->getSegment(order.at(orderPos - 1)).endJoints;
}

void PlaybackEngine::buildKeyframes()
{
    m_keys.clear();
    if (!m_model) {
        return;
    }

    // 只有 spline 的「机械臂」片段才需要全局关键帧；其余情况走逐段插值。
    // 夹爪片段不参与关节插值，混进来会让 Catmull-Rom 把手臂拽向零位。
    bool needsSpline = false;
    for (int i = 0; i < m_model->segmentCount(); ++i) {
        const Segment seg = m_model->getSegment(i);
        if (seg.isArm() && seg.interpolationType == QLatin1String("spline")) {
            needsSpline = true;
            break;
        }
    }
    if (!needsSpline) {
        return;
    }

    // 按时间轴顺序收集关键帧：本段起点取「时间上前一段的终点」，
    // 首段用冻结的出发姿态。顺序反了会让 spline 的区间定位失效。
    const QVector<int> order = armIndicesByTime();
    for (int k = 0; k < order.size(); ++k) {
        const Segment seg = m_model->getSegment(order.at(k));

        // 首段起点必须走 armSegmentStart()，而不是 TimelineModel::segmentStartJoints()：
        // 后者返回实时更新的 m_originPose，播放中会跟着实测角漂，
        // spline 的整条轨迹因此逐帧改写，真机上表现为模型轨迹抖跳。
        Keyframe k0;
        k0.time = seg.startTime;
        k0.state = armSegmentStart(order, k);
        m_keys.append(k0);

        Keyframe k1;
        k1.time = seg.startTime + seg.duration;
        k1.state = seg.endJoints;
        m_keys.append(k1);
    }

    // 片段可以在时间轴上互相拖动重叠，收集出来的顺序不一定按时间递增。
    // Catmull-Rom 的区间定位（while t > times[i+1]）假设时间单调递增，
    // 乱序会让它定位到错误的区间，插值结果完全跑偏。
    std::stable_sort(m_keys.begin(), m_keys.end(),
                     [](const Keyframe &a, const Keyframe &b) {
                         return a.time < b.time;
                     });

    // 首尾相连的两段会在交界处产生一对重合的关键帧：
    // 前一段的终点与后一段的起点，同一时刻、同一姿态。
    //
    // 必须去掉重复点。Catmull-Rom 求切线要找前后邻居，遇到重复点时
    // 交界处的切线会从 0.5*(C-A) 退化成 0.5*(C-B)——
    // 速度虽然仍连续（两侧算出来一样），但幅值偏小，曲率不对，
    // 滑动到交界处能看出一个轻微的顿挫。
    QVector<Keyframe> merged;
    merged.reserve(m_keys.size());
    for (const Keyframe &k : m_keys) {
        if (!merged.isEmpty()) {
            const Keyframe &prev = merged.last();
            const bool sameTime = qAbs(k.time - prev.time) < 1e-6;
            bool samePose = sameTime;
            if (sameTime) {
                for (int d = 0; d < 7; ++d) {
                    if (qAbs(k.state.j[d] - prev.state.j[d]) > 1e-9
                        || qAbs(k.state.jr[d] - prev.state.jr[d]) > 1e-9) {
                        samePose = false;
                        break;
                    }
                }
            }
            // 同一时刻但姿态不同（重叠片段各自的目标冲突）不能合并，
            // 那是真实的不连续，强行合成一个点会篡改轨迹。
            if (samePose) {
                continue;
            }
        }
        merged.append(k);
    }
    m_keys.swap(merged);
}

JointState PlaybackEngine::originPose() const
{
    if (m_originFrozen) {
        return m_originSnapshot;
    }
    return m_model ? m_model->originPose() : JointState();
}

JointState PlaybackEngine::evaluateAt(double time) const
{
    if (!m_model || m_model->segmentCount() == 0) {
        return JointState();
    }
    if (time <= 0.0) {
        return originPose();
    }

    // 夹爪片段不需要单独处理：下面的循环只看机械臂片段。
    // 若此刻落在夹爪片段里（没有机械臂片段覆盖），循环会走到「空隙」分支，
    // 返回上一个机械臂片段的终点 —— 正是「保持不动」的语义。
    // 若夹爪片段与机械臂片段在时间上重叠，循环照常插值，手臂继续走它的轨迹。

    // 按时间轴位置遍历机械臂片段，而不是列表顺序：片段可以被拖到任意位置，
    // 「上一段」指的是时间上紧邻的那一段，不是数组里的前一项。
    const QVector<int> order = armIndicesByTime();
    if (order.isEmpty()) {
        return originPose();        // 只有夹爪片段：手臂保持出发姿态
    }

    for (int k = 0; k < order.size(); ++k) {
        const Segment seg = m_model->getSegment(order.at(k));
        const double segEnd = seg.startTime + seg.duration;
        // 本段的起点是「时间上位于它之前的机械臂片段的终点」，
        // 首段则用冻结的出发姿态，播放中不会跟着实测角漂移
        const JointState segStart = armSegmentStart(order, k);

        if (time >= seg.startTime && time < segEnd) {
            const double span = qMax(1e-6, seg.duration);
            const double localT = (time - seg.startTime) / span;

            if (seg.interpolationType == QLatin1String("linear")) {
                return InterpolationEngine::linearInterpolate(
                    segStart, seg.endJoints, localT);
            }
            if (seg.interpolationType == QLatin1String("spline")) {
                // 用整条轨迹的关键帧求跨段连续的速度
                if (m_keys.size() >= 2) {
                    QVector<JointState> states;
                    QVector<double> times;
                    states.reserve(m_keys.size());
                    times.reserve(m_keys.size());
                    for (const Keyframe &k : m_keys) {
                        states.append(k.state);
                        times.append(k.time);
                    }
                    return InterpolationEngine::cubicSplineInterpolate(states, times, time);
                }
                return InterpolationEngine::smoothStepInterpolate(
                    segStart, seg.endJoints, localT);
            }
            // 默认 cubic
            return InterpolationEngine::smoothStepInterpolate(
                segStart, seg.endJoints, localT);
        }

        // 落在两个机械臂片段之间的空隙（可能夹着夹爪片段）：保持上一段的结束姿态。
        // 这里按时间序判断：落在本段起点之前，说明处在「上一段结束 ~ 本段开始」的空隙里，
        // 保持的正是本段的起点。
        if (time < seg.startTime) {
            return segStart;
        }
    }

    // 超出时长：停在时间上最后一个机械臂片段的结束姿态。
    return m_model->getSegment(order.last()).endJoints;
}

// ---------------------------------------------------------------- 控制

void PlaybackEngine::primeStartFrame()
{
    if (!m_model || m_model->segmentCount() == 0) {
        return;
    }
    // 冻结起点：整条轨迹的插值基准必须是这一刻的实测姿态，
    // 后续实测角再变化也不会影响已经算好的轨迹。
    m_originSnapshot = m_model->originPose();
    m_originFrozen = true;
    buildKeyframes();

    m_accumulatedMs = 0.0;
    m_lastEmittedTime = -1.0;
    m_currentSegment = -1;

    // 只发这一帧，不启动定时器——时间轴不动
    emitFrameAt(0.0);
    emit progressChanged(0.0, totalDuration());

    // 标记快照已就绪，随后 play() 不能再覆盖它
    m_primed = true;
}

void PlaybackEngine::play()
{
    if (!m_model || m_model->segmentCount() == 0) {
        qWarning() << "PlaybackEngine: 没有可播放的片段";
        return;
    }

    // 起点冻结：已经有预先对齐好的快照（primeStartFrame）就沿用它。
    // 重新取一次实时角会把对齐结果覆盖掉——那 700ms 里机器人已经
    // 被带到轨迹起点了，再拿新实测值当基准会把轨迹整体平移，产生跳变。
    if (!m_primed) {
        m_originSnapshot = m_model->originPose();
    }
    m_originFrozen = true;
    m_primed = false;

    buildKeyframes();

    m_state = Playing;
    m_accumulatedMs = 0.0;
    m_lastEmittedTime = -1.0;
    m_currentSegment = -1;      // 从 0 号片段重新开始，保证首段的灵巧手动作会发
    m_clock.restart();
    m_timer->start();

    emit started();
    emit progressChanged(0.0, totalDuration());

    // 立即发出第 0 帧，避免首帧延迟
    emitFrameAt(0.0);
}

void PlaybackEngine::pause()
{
    if (m_state != Playing) {
        return;
    }
    m_timer->stop();
    m_state = Paused;
    emit paused();
}

void PlaybackEngine::resume()
{
    if (m_state != Paused) {
        return;
    }
    m_clock.restart();
    m_timer->start();
    m_state = Playing;
    emit resumed();
}

void PlaybackEngine::stop()
{
    const bool wasActive = (m_state != Idle);
    m_timer->stop();
    m_state = Idle;
    m_accumulatedMs = 0.0;
    m_lastEmittedTime = -1.0;
    m_currentSegment = -1;
    m_originFrozen = false;     // 停止后起点重新跟随实测角
    m_primed = false;
    if (wasActive) {
        emit stopped();
    }
}

void PlaybackEngine::seek(double time)
{
    const double total = totalDuration();
    m_accumulatedMs = qBound(0.0, time, total) * 1000.0;
    if (m_state == Playing) {
        m_clock.restart();
    }
    emitFrameAt(currentTime());
    emit progressChanged(currentTime(), total);
}

// ---------------------------------------------------------------- 主循环

void PlaybackEngine::onTick()
{
    if (m_state != Playing) {
        return;
    }

    const double total = totalDuration();
    if (total <= 0.0) {
        stop();
        return;
    }

    // 按真实经过时间推进，而不是假定每帧正好 16 ms，
    // 这样即使某一帧卡住，播放进度也不会落后。
    // 固定 1 倍速：时间轴上 1 秒就是现实 1 秒。
    const qint64 deltaMs = m_clock.restart();
    m_accumulatedMs += deltaMs;

    double t = currentTime();
    if (t >= total) {
        // 循环：回绕到起点接着跑，不重冻结 m_originSnapshot，
        // 保证每一圈走的是同一条轨迹。
        if (m_loop) {
            // 把超出的部分带到下一圈，避免慢速回绕导致周期变长
            const double overrun = m_accumulatedMs - total * 1000.0;
            m_accumulatedMs = qMax(0.0, overrun);
            m_lastEmittedTime = -1.0;
            // 重置段号：单段循环时回绕后仍是 0 号段，不重置就不会再发灵巧手动作
            m_currentSegment = -1;
            const double wrapped = currentTime();
            emit looped();
            emitFrameAt(wrapped);
            emit progressChanged(wrapped, total);
            return;
        }

        emitFrameAt(total);
        emit progressChanged(total, total);
        m_timer->stop();
        m_state = Idle;
        if (m_rewindOnFinish) {
            m_accumulatedMs = 0.0;
        } else {
            m_accumulatedMs = total * 1000.0;
        }
        m_lastEmittedTime = -1.0;
        m_originFrozen = false;
        m_primed = false;
        emit finished();
        return;
    }

    emitFrameAt(t);
    emit progressChanged(t, total);
}

void PlaybackEngine::emitFrameAt(double time)
{
    // 同一时刻不重复发，省掉无意义的 UDP 包和重绘
    if (qFuzzyCompare(time + 1.0, m_lastEmittedTime + 1.0)) {
        return;
    }
    m_lastEmittedTime = time;
    updateSegmentTracking(time);
    emit frameReady(time, evaluateAt(time));
}

int PlaybackEngine::segmentIndexAt(double time) const
{
    return m_model ? m_model->segmentIndexAt(time) : -1;
}

void PlaybackEngine::updateSegmentTracking(double time)
{
    const int idx = segmentIndexAt(time);
    if (idx == m_currentSegment) {
        return;
    }
    m_currentSegment = idx;
    emit segmentEntered(idx);
}
