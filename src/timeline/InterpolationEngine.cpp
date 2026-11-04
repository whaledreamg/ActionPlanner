#include "timeline/InterpolationEngine.h"
#include <QtMath>

JointState InterpolationEngine::lerp(const JointState& a, const JointState& b, double t)
{
    JointState r;
    for (int i = 0; i < 7; ++i) {
        r.j[i]  = a.j[i]  + t * (b.j[i]  - a.j[i]);
        r.jr[i] = a.jr[i] + t * (b.jr[i] - a.jr[i]);
    }
    return r;
}

JointState InterpolationEngine::linearInterpolate(const JointState& start,
                                                  const JointState& end,
                                                  double t)
{
    return lerp(start, end, qBound(0.0, t, 1.0));
}

JointState InterpolationEngine::smoothStepInterpolate(const JointState& start,
                                                      const JointState& end,
                                                      double t)
{
    // 归一化后做 3t²-2t³：起止速度为 0，中间加速再减速。
    // 相比线性插值，点位动作不会在两端"顿一下"。
    const double u = qBound(0.0, t, 1.0);
    const double s = u * u * (3.0 - 2.0 * u);
    return lerp(start, end, s);
}

JointState InterpolationEngine::cubicSplineInterpolate(const QVector<JointState>& keyframes,
                                                       const QVector<double>& times,
                                                       double t)
{
    const int n = qMin(keyframes.size(), times.size());
    if (n == 0) {
        return JointState();
    }
    if (n == 1) {
        return keyframes.first();
    }
    if (t <= times.first()) {
        return keyframes.first();
    }
    if (t >= times[n - 1]) {
        return keyframes[n - 1];
    }

    // 定位所在的时间区间
    int i = 0;
    while (i < n - 2 && t > times[i + 1]) {
        ++i;
    }

    const double t0 = times[i];
    const double t1 = times[i + 1];
    const double span = t1 - t0;
    if (span <= 1e-9) {
        return keyframes[i + 1];
    }
    const double u = (t - t0) / span;

    // 用「按时间尺度算切线」的 Hermite 形式，而不是教科书上的均匀
    // Catmull-Rom 基函数。
    //
    // 均匀 Catmull-Rom 假定相邻控制点等间隔，在 u=0 处的速度是
    // 0.5*(p2-p0)/span。片段时长不一样时（一段 1s、一段 3s），
    // 同一个交界点两侧的 span 不同，算出的速度差好几倍 ——
    // 本该连续的位置出现速度跳变，手臂会「抽」一下。
    //
    // 把切线直接定义成时间域的速度（对时间求导），再代入 Hermite 基函数，
    // 无论时长怎么分布，交界处两侧速度都严格相等。
    const JointState& p1 = keyframes[i];
    const JointState& p2 = keyframes[i + 1];

    // 端点没有一侧邻居，退化成单边差分，仍是该区间内的正确速度
    const double dtIn1  = (i > 0) ? (times[i + 1] - times[i - 1]) : span;
    const double dtIn2  = (i + 2 < n) ? (times[i + 2] - times[i]) : span;
    const JointState& pPrev = (i > 0) ? keyframes[i - 1] : p1;
    const JointState& pNext = (i + 2 < n) ? keyframes[i + 2] : p2;

    // Hermite 基函数
    const double u2 = u * u;
    const double u3 = u2 * u;
    const double h00 =  2.0 * u3 - 3.0 * u2 + 1.0;
    const double h10 =        u3 - 2.0 * u2 + u;
    const double h01 = -2.0 * u3 + 3.0 * u2;
    const double h11 =        u3 -       u2;

    JointState r;
    for (int k = 0; k < 7; ++k) {
        // 左右臂分别算：切线是该关节角对时间的变化率（rad/s）
        const double m1j  = (p2.j[k]  - pPrev.j[k]) / dtIn1;
        const double m2j  = (pNext.j[k] - p1.j[k]) / dtIn2;
        r.j[k] = h00 * p1.j[k] + h10 * span * m1j
               + h01 * p2.j[k] + h11 * span * m2j;

        const double m1r  = (p2.jr[k]  - pPrev.jr[k]) / dtIn1;
        const double m2r  = (pNext.jr[k] - p1.jr[k]) / dtIn2;
        r.jr[k] = h00 * p1.jr[k] + h10 * span * m1r
                + h01 * p2.jr[k] + h11 * span * m2r;
    }
    return r;
}
