#ifndef TIMELINE_MODEL_H
#define TIMELINE_MODEL_H

#include <QObject>
#include <QList>
#include <QString>

struct JointState {
    double j[7];  // Left arm joints
    double jr[7]; // Right arm joints

    JointState() {
        for (int i = 0; i < 7; ++i) {
            j[i] = 0.0;
            jr[i] = 0.0;
        }
    }
};

/** 七轴关节角度限位（弧度）。默认给 ±180°，实际值由 URDF 提供。 */
struct JointLimits {
    double lower[7];
    double upper[7];

    JointLimits() {
        for (int i = 0; i < 7; ++i) {
            lower[i] = -3.14159265358979;
            upper[i] =  3.14159265358979;
        }
    }
};

/**
 * 灵巧手目标姿态：6 个自由度，归一化 0~1000。
 *
 * 与机械臂关节角不同，灵巧手不插值也不跟随时间连续变化——
 * 一段里只下发一次，驱动器自己按 writeTime 走完。所以存的是「目标值」
 * 而不是轨迹。
 */
struct HandPose {
    static constexpr int kDofCount = 6;

    int dof[kDofCount];

    HandPose() {
        for (int i = 0; i < kDofCount; ++i) {
            dof[i] = 0;
        }
    }

    bool operator==(const HandPose &o) const {
        for (int i = 0; i < kDofCount; ++i) {
            if (dof[i] != o.dof[i]) {
                return false;
            }
        }
        return true;
    }
    bool operator!=(const HandPose &o) const { return !(*this == o); }
};

/**
 * 片段类型。两类片段共用同一条时间轴轨道，但语义完全独立：
 * 机械臂片段驱动关节轨迹，夹爪片段只驱动灵巧手、机械臂保持不动。
 *
 * 分开的理由：机械臂是连续轨迹，要插值；灵巧手是离散动作，只在进入片段时
 * 下发一次目标，之后整段保持。把两者塞进同一个片段结构，就会出现
 * 「想单独插一个换手动作，却被迫给机械臂也指定一个目标姿态」的别扭。
 */
enum class SegmentType {
    Arm,    ///< 机械臂关节轨迹片段
    Hand    ///< 灵巧手动作片段（机械臂保持不动）
};

/**
 * 一个动作片段。
 *
 * 机械臂片段（Arm）只记录「结束关节角」：片段首尾相连，本段的起点就是
 * 上一段的终点。这样改前一段的终点时，后一段的起点自动跟着变，不会出现
 * 前后矛盾。第一段没有「上一段」，它的起点取机械臂的实时姿态（originPose）。
 * 这个值不入文件——每次从当前实际位置出发，避免轨迹把机器人拽到某个
 * 固定姿态上去。
 *
 * 灵巧手片段（Hand）只记录手位，不碰机械臂：endJoints 被忽略，
 * 机械臂在整段期间保持进入本段之前的姿态。
 */
struct Segment {
    SegmentType type = SegmentType::Arm;

    QString label;
    double startTime = 0.0;
    double duration = 1.0;

    /** 仅 Arm 片段有意义 */
    JointState endJoints;
    QString interpolationType = QStringLiteral("cubic"); // "linear", "cubic", "spline"

    /** 仅 Hand 片段有意义 */
    HandPose leftHand;
    HandPose rightHand;

    bool isArm() const { return type == SegmentType::Arm; }
    bool isHand() const { return type == SegmentType::Hand; }
};

class TimelineModel : public QObject
{
    Q_OBJECT

public:
    explicit TimelineModel(QObject *parent = nullptr);

    void addSegment(const Segment& segment);
    void removeSegment(int index);
    void updateSegment(int index, const Segment& segment);
    Segment getSegment(int index) const;
    int segmentCount() const;
    double totalDuration() const;

    /**
     * 第 index 段的起始关节角。
     *
     * 只沿「机械臂片段」回溯：夹爪片段不产生关节目标，若把它算进来，
     * 夹爪片段之后的手臂片段起点会被错当成一个空 JointState，
     * 播放时手臂会突然跳回零位。
     *
     * index == 0 或前面没有任何机械臂片段时返回 originPose()。
     */
    JointState segmentStartJoints(int index) const;

    /** 该片段之前（不含自己）最后一个机械臂片段的结束角 */
    JointState previousArmEnd(int index) const;

    /**
     * 第一段的起点，也就是整条轨迹的出发姿态。
     * 由实测关节角实时更新，不落盘。
     */
    JointState originPose() const { return m_originPose; }
    void setOriginPose(const JointState& pose);

    /** 该时刻落在哪个片段，没有返回 -1 */
    int segmentIndexAt(double time) const;

signals:
    void originPoseChanged(const JointState& pose);

    /**
     * 片段内容、数量或顺序发生变化。
     *
     * spline 插值依赖全局关键帧，关键帧必须跟着片段改动重建；
     * 否则编辑完片段、不点播放只拖动游标时，算出来的还是旧轨迹。
     */
    void segmentsChanged();

private:
    void updateTotalDuration();

    QList<Segment> m_segments;
    JointState m_originPose;
    double m_totalDuration;
};

#endif
