# Joint Origin RPY修复

## 核心问题

之前STL模型的连接不正确，因为我们**完全忽略了joint的origin RPY（旋转）**！

## 参考实现对比

### xcoredatatool中的正确实现：

```cpp
void URDFModel::calculateLinkTransform(const QString &linkName,
                                       const QMatrix4x4 &parentTransform)
{
    QMatrix4x4 transform = parentTransform;

    // 查找父关节
    for (auto it = joints_.begin(); it != joints_.end(); ++it) {
        if (it->child == linkName) {
            const URDFJoint &joint = it.value();

            // 1. 应用关节原点平移
            transform.translate(joint.origin_xyz);

            // 2. 应用关节原点旋转 (RPY)  ← 我们之前缺了这个！
            if (!joint.origin_rpy.isNull()) {
                transform.rotate(R2D(joint.origin_rpy.z()), 0, 0, 1);  // Yaw
                transform.rotate(R2D(joint.origin_rpy.y()), 0, 1, 0);  // Pitch
                transform.rotate(R2D(joint.origin_rpy.x()), 1, 0, 0);  // Roll
            }

            // 3. 应用关节运动
            if (joint.type == "revolute" || joint.type == "continuous") {
                transform.rotate(R2D(joint.currentAngle), joint.axis);
            }
            break;
        }
    }
    ...
}
```

### 我们之前的错误实现：

```cpp
void RobotRenderer::calculateLinkTransformRecursive(...) {
    QMatrix4x4 transform = parentTransform;
    
    for (const Joint& joint : m_joints) {
        if (joint.childLink == linkName) {
            // 只有平移！
            transform.translate(joint.origin);
            
            // 缺少origin RPY旋转！❌
            
            // 应用关节角度
            transform.rotate(angleDeg, joint.axis);
            break;
        }
    }
    ...
}
```

## 修复内容

### 1. Joint结构添加originRPY字段

```cpp
struct Joint {
    QString name;
    QString type;
    QString parentLink;
    QString childLink;
    QVector3D origin;      // Joint的位置偏移 (xyz)
    QVector3D originRPY;   // Joint的旋转偏移 (roll, pitch, yaw 弧度) ← 新增
    QVector3D axis;
    double lowerLimit;
    double upperLimit;
    double currentAngle = 0.0;
};
```

### 2. URDFLoader解析joint的origin rpy

```cpp
XMLElement* origin = jointElem->FirstChildElement("origin");
if (origin) {
    // 解析xyz
    const char* xyz = origin->Attribute("xyz");
    if (xyz) {
        sscanf(xyz, "%f %f %f", &x, &y, &z);
        joint.origin = QVector3D(x, y, z);
    }
    
    // 解析rpy ← 新增
    const char* rpy = origin->Attribute("rpy");
    if (rpy) {
        sscanf(rpy, "%f %f %f", &r, &p, &yaw);
        joint.originRPY = QVector3D(r, p, yaw);
    }
}
```

### 3. RobotRenderer应用origin RPY变换

```cpp
void RobotRenderer::calculateLinkTransformRecursive(...) {
    QMatrix4x4 transform = parentTransform;
    
    for (const Joint& joint : m_joints) {
        if (joint.childLink == linkName) {
            // 1. 应用joint origin平移
            transform.translate(joint.origin);
            
            // 2. 应用joint origin旋转 (RPY: Roll-Pitch-Yaw, ZYX顺序) ← 新增
            if (!joint.originRPY.isNull()) {
                transform.rotate(qRadiansToDegrees(joint.originRPY.z()), 0, 0, 1);  // Yaw (Z)
                transform.rotate(qRadiansToDegrees(joint.originRPY.y()), 0, 1, 0);  // Pitch (Y)
                transform.rotate(qRadiansToDegrees(joint.originRPY.x()), 1, 0, 0);  // Roll (X)
            }
            
            // 3. 应用关节运动角度
            if (joint.type == "revolute" || joint.type == "continuous") {
                if (i < m_jointAngles.size() && !joint.axis.isNull()) {
                    double angleDeg = qRadiansToDegrees(m_jointAngles[i]);
                    transform.rotate(angleDeg, joint.axis);
                }
            }
            break;
        }
    }
    
    linkTransforms[linkName] = transform;
    
    // 递归处理子link
    ...
}
```

## 完整的变换链

现在每个link的完整变换矩阵计算为：

```
LinkTransform = ParentLinkTransform 
                × JointOriginTranslation 
                × JointOriginRotation (RPY)  ← 之前缺了这个
                × JointAngleRotation
```

然后渲染mesh时还要应用visual origin：

```
FinalTransform = LinkTransform 
                 × VisualOriginTranslation
                 × VisualOriginRotation (RPY)
```

## 为什么RPY很重要？

在URDF中，很多joint的origin不仅有位置偏移，还有旋转偏移。比如：

```xml
<joint name="left_j2_joint" type="revolute">
    <origin rpy="1.57 0 0" xyz="0 0 0"/>  ← rpy="1.57 0 0" 表示绕X轴旋转90度
    <parent link="left_j1_Link"/>
    <child link="left_j2_Link"/>
    <axis xyz="0 1 0"/>
</joint>
```

如果忽略这个rpy，link就会在错误的姿态下渲染，导致整个机械臂结构扭曲！

## 测试

```bash
cd /home/gxh/workspace/project/ActionPlanner
./ActionPlanner
```

点击"加载模型"，现在终端会输出每个joint的originRPY：
```
URDFLoader: Added joint: "left_j1_joint" origin: QVector3D(0, 0, 0.195) originRPY: QVector3D(0, 0, 0)
URDFLoader: Added joint: "left_j2_joint" origin: QVector3D(0, 0, 0) originRPY: QVector3D(1.57, 0, 0)
```

现在STL模型应该正确连接了！✅
