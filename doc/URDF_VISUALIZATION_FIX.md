# URDF可视化修复 - 参考xcoredatatool实现

## 问题分析

参考了`/home/gxh/workspace/project/xcoredatatool/form/sim/`下的URDF仿真实现，发现之前的变换计算逻辑有误。

## 关键修复

### 1. 正确的运动学树遍历

**之前的错误**：
- 简单累加所有joint的origin
- 假设joints按顺序排列
- 没有考虑parent-child关系

**参考xcoredatatool的实现**：
```cpp
// urdfmodel.cpp: calculateLinkTransform
void URDFModel::calculateLinkTransform(const QString &linkName,
                                       const QMatrix4x4 &parentTransform)
{
    QMatrix4x4 transform = parentTransform;
    
    // 查找父关节
    for (auto it = joints_.begin(); it != joints_.end(); ++it) {
        if (it->child == linkName) {
            const URDFJoint &joint = it.value();
            
            // 应用关节原点变换
            transform.translate(joint.origin_xyz);
            
            // 应用关节旋转(RPY)
            if (!joint.origin_rpy.isNull()) {
                transform.rotate(R2D(joint.origin_rpy.z()), 0, 0, 1);
                transform.rotate(R2D(joint.origin_rpy.y()), 0, 1, 0);
                transform.rotate(R2D(joint.origin_rpy.x()), 1, 0, 0);
            }
            
            // 应用关节运动
            if (joint.type == "revolute" || joint.type == "continuous") {
                transform.rotate(R2D(joint.currentAngle), 
                               joint.axis.x(), joint.axis.y(), joint.axis.z());
            }
            break;
        }
    }
    
    transforms_[linkName] = transform;
    
    // 递归处理子链接
    for (auto it = joints_.begin(); it != joints_.end(); ++it) {
        if (it->parent == linkName) {
            calculateLinkTransform(it->child, transform);
        }
    }
}
```

### 2. 新的实现

```cpp
void RobotRenderer::updateVisualization() {
    // 1. 找到root link (没有任何joint指向它作为child的link)
    QSet<QString> childLinks;
    for (const Joint& joint : m_joints) {
        childLinks.insert(joint.childLink);
    }
    
    QString rootLink;
    for (const Link& link : m_links) {
        if (!childLinks.contains(link.name)) {
            rootLink = link.name;
            break;
        }
    }
    
    // 2. 从root开始递归计算所有link的变换
    QMap<QString, QMatrix4x4> linkTransforms;
    calculateLinkTransformRecursive(rootLink, QMatrix4x4(), linkTransforms);
    
    // 3. 提取每个joint的child link位置
    for (const Joint& joint : m_joints) {
        QMatrix4x4 childTransform = linkTransforms[joint.childLink];
        Transform t;
        t.position = QVector3D(childTransform(0, 3), 
                              childTransform(1, 3), 
                              childTransform(2, 3));
        m_jointTransforms.append(t);
    }
}

void RobotRenderer::calculateLinkTransformRecursive(
    const QString& linkName, 
    const QMatrix4x4& parentTransform,
    QMap<QString, QMatrix4x4>& linkTransforms) 
{
    QMatrix4x4 transform = parentTransform;
    
    // 查找连接到这个link作为child的joint
    for (const Joint& joint : m_joints) {
        if (joint.childLink == linkName) {
            // 应用joint的origin平移
            transform.translate(joint.origin);
            
            // 应用关节角度
            if (joint.type == "revolute" || joint.type == "continuous") {
                double angleDeg = qRadiansToDegrees(joint.currentAngle);
                transform.rotate(angleDeg, joint.axis);
            } else if (joint.type == "prismatic") {
                transform.translate(joint.axis * joint.currentAngle);
            }
            break;
        }
    }
    
    // 保存变换
    linkTransforms[linkName] = transform;
    
    // 递归处理子链接
    for (const Joint& joint : m_joints) {
        if (joint.parentLink == linkName) {
            calculateLinkTransformRecursive(joint.childLink, transform, linkTransforms);
        }
    }
}
```

## 核心差异

| 方面 | 之前的实现 | xcoredatatool实现 | 新实现 |
|------|-----------|-----------------|--------|
| **树遍历** | 顺序遍历joints | 递归遍历link树 | ✅ 递归遍历link树 |
| **变换组合** | 简单累加origin | parent×origin×rotation | ✅ parent×origin×rotation |
| **root link识别** | 假设第一个 | 查找没有parent的link | ✅ 查找child集合外的link |
| **关节角度应用** | 在joint位置旋转 | 在link变换中旋转 | ✅ 在link变换中旋转 |

## 运行结果

现在终端输出应该显示：
```
Found root link: "world"
Joint 0 "left_j1_joint" child: "left_j1_Link" at QVector3D(0, 0, 0.195)
Joint 1 "left_j2_joint" child: "left_j2_Link" at QVector3D(0, 0, 0.195)
Joint 2 "left_j3_joint" child: "left_j3_Link" at QVector3D(0, 0.2725, 0.195)
...
```

3D窗口应该正确显示：
- ✅ 左臂从base向上延伸
- ✅ 右臂从base向上延伸
- ✅ 关节之间的正确间距
- ✅ 可以用鼠标旋转查看

## 编译运行

```bash
cd /home/gxh/workspace/project/ActionPlanner
make -j4
./ActionPlanner
```

点击"加载模型"按钮，现在应该能看到正确的机器人模型结构了！
