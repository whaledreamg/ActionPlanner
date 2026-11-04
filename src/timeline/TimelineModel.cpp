#include "timeline/TimelineModel.h"

#include <QDebug>
#include <algorithm>

TimelineModel::TimelineModel(QObject *parent)
    : QObject(parent)
    , m_totalDuration(0.0)
{
}

void TimelineModel::addSegment(const Segment& segment) {
    m_segments.append(segment);
    updateTotalDuration();
    emit segmentsChanged();
}

void TimelineModel::removeSegment(int index) {
    if (index >= 0 && index < m_segments.size()) {
        m_segments.removeAt(index);
        updateTotalDuration();
        emit segmentsChanged();
    }
}

Segment TimelineModel::getSegment(int index) const {
    return m_segments.value(index);
}

void TimelineModel::updateSegment(int index, const Segment& segment) {
    if (index >= 0 && index < m_segments.size()) {
        m_segments[index] = segment;
        updateTotalDuration();
        emit segmentsChanged();
    }
}

int TimelineModel::segmentCount() const {
    return m_segments.size();
}

double TimelineModel::totalDuration() const {
    return m_totalDuration;
}

JointState TimelineModel::segmentStartJoints(int index) const
{
    return previousArmEnd(index);
}

JointState TimelineModel::previousArmEnd(int index) const
{
    // index 允许等于 segmentCount()：那是「即将追加到末尾的新片段」，
    // 此时没有自己的位置，前驱就是时间轴上最靠后的机械臂片段。
    if (index < 0 || index > m_segments.size()) {
        return m_originPose;
    }
    const bool appending = (index == m_segments.size());

    // 「上一段」必须按时间轴位置判定，而不是数组下标。
    // 片段可以被拖到任意位置，segmentCount()-1 未必是时间上最靠后的那段；
    // 按数组下标回溯，拖动过片段后就会取到错误的起点，播放时手臂瞬间跳位。
    //
    // 排序规则必须与 PlaybackEngine::armIndicesByTime() 完全一致
    // （按 startTime 升序，相同则保持数组先后），否则两处对同一个片段
    // 会取到不同的前驱，求值和关键帧就对不上了。
    QVector<int> order;
    order.reserve(m_segments.size());
    for (int i = 0; i < m_segments.size(); ++i) {
        // 夹爪片段没有关节目标，不参与链式推导；否则手臂会被拽向零位
        if (m_segments.at(i).isArm()) {
            order.append(i);
        }
    }
    std::stable_sort(order.begin(), order.end(), [this](int a, int b) {
        return m_segments.at(a).startTime < m_segments.at(b).startTime;
    });

    int pos = -1;
    if (appending) {
        pos = order.size();         // 「追加」等价于排在时间序最末
    } else {
        pos = order.indexOf(index);
        if (pos < 0) {
            return m_originPose;    // 自身是夹爪片段，不产生关节起点
        }
    }
    if (pos <= 0) {
        return m_originPose;
    }
    return m_segments.at(order.at(pos - 1)).endJoints;
}

int TimelineModel::segmentIndexAt(double time) const
{
    for (int i = 0; i < m_segments.size(); ++i) {
        const Segment &seg = m_segments.at(i);
        if (time >= seg.startTime && time < seg.startTime + seg.duration) {
            return i;
        }
    }
    return -1;
}

void TimelineModel::setOriginPose(const JointState& pose)
{
    m_originPose = pose;
    emit originPoseChanged(m_originPose);
}

void TimelineModel::updateTotalDuration() {
    m_totalDuration = 0.0;
    for (const Segment& seg : m_segments) {
        m_totalDuration = qMax(m_totalDuration, seg.startTime + seg.duration);
    }
}
