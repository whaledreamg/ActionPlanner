#ifndef INTERPOLATION_ENGINE_H
#define INTERPOLATION_ENGINE_H

#include "TimelineModel.h"
#include <QVector>

/**
 * 关节角插值。
 *
 * 三种插值方式对应三种不同的运动品质：
 *  - linear：匀速直线，速度在片段边界突变（会有抖动/冲击）
 *  - cubic ：片段内 S 形加减速，起止速度为零，适合点位动作
 *  - spline：跨片段的三次样条，关键帧处速度连续，
 *            适合连续的轨迹（不会在每段边界停顿一下）
 */
class InterpolationEngine
{
public:
    /** 线性，用于 "linear" */
    static JointState linearInterpolate(const JointState& start,
                                        const JointState& end,
                                        double t);

    /** 片段内三次 S 曲线（零起止速度），用于 "cubic" */
    static JointState smoothStepInterpolate(const JointState& start,
                                            const JointState& end,
                                            double t);

    /**
     * 跨关键帧的样条插值，用于 "spline"。
     *
     * 内部用按时间尺度的 Hermite 形式（等价于非均匀 Catmull-Rom），
     * 因此关键帧间隔不等时，交界处速度依然连续。这正是它和
     * smoothStep 的区别：smoothStep 在每段两端都把速度归零，
     * 连续轨迹会在每个段边界顿一下。
     *
     * keyframes/times 需按时间升序且时刻互不相同；内部会做端点钳制。
     */
    static JointState cubicSplineInterpolate(const QVector<JointState>& keyframes,
                                             const QVector<double>& times,
                                             double t);

private:
    static JointState lerp(const JointState& a, const JointState& b, double t);
};

#endif
