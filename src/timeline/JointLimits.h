#ifndef JOINTLIMITS_H
#define JOINTLIMITS_H

#include "timeline/TimelineModel.h"

#include <QString>

/**
 * 从 URDF 读取七轴关节限位，供片段编辑对话框做范围约束。
 *
 * URDF 是运行时从磁盘加载的，路径可能与程序工作目录不同，因此这里做一次
 * 按常见候选路径的查找；找不到就退回 ±180° 默认值，不影响编辑功能。
 */
class JointLimitsLoader
{
public:
    struct Limits {
        JointLimits left;
        JointLimits right;
        bool loaded = false;   // 是否真的读到了 URDF
    };

    /** 读取限位。结果带缓存，重复调用不会反复解析文件。 */
    static Limits load();

    /** 指定 URDF 路径（可选），不设则自动查找。 */
    static void setUrdfPath(const QString& path);

private:
    static QString findUrdf();
};

#endif // JOINTLIMITS_H
