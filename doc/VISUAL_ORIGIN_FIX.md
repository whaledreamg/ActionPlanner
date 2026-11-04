# Visual Origin变换修复

## 问题

之前STL模型显示了，但是各个link之间的连接位置不对，它们看起来像是分离的。

## 原因

在URDF中，每个`<link>`的`<visual>`元素都有自己的`<origin>`变换：

```xml
<link name="left_j1_Link">
    <visual>
        <origin rpy="0 0 0" xyz="0 0 0"/>  ← 这个origin是相对于link坐标系的偏移
        <geometry>
            <mesh filename="package://meshes/left_j1_Link.STL"/>
        </geometry>
    </visual>
</link>
```

之前的代码只考虑了joint的origin，忽略了visual的origin，导致mesh在错误的位置渲染。

## 修复

### 1. 在Link结构中添加visual origin字段

```cpp
struct Link {
    ...
    // Visual元素的origin变换 (相对于link坐标系)
    QVector3D visualOrigin;
    QVector3D visualRPY;  // Roll, Pitch, Yaw (弧度)
};
```

### 2. URDFLoader解析visual的origin

```cpp
XMLElement* visual = linkElem->FirstChildElement("visual");
if (visual) {
    XMLElement* origin = visual->FirstChildElement("origin");
    if (origin) {
        // 解析xyz
        const char* xyz = origin->Attribute("xyz");
        if (xyz) {
            sscanf(xyz, "%f %f %f", &x, &y, &z);
            link.visualOrigin = QVector3D(x, y, z);
        }
        
        // 解析rpy
        const char* rpy = origin->Attribute("rpy");
        if (rpy) {
            sscanf(rpy, "%f %f %f", &r, &p, &yaw);
            link.visualRPY = QVector3D(r, p, yaw);
        }
    }
}
```

### 3. 渲染时应用visual origin变换

```cpp
void RobotRenderer::renderMesh(const QString& linkName, const QMatrix4x4& transform) {
    glPushMatrix();
    
    // 应用link的变换
    glMultMatrixf(transform.constData());
    
    // 应用visual元素的origin变换
    glTranslatef(link->visualOrigin.x(), 
                 link->visualOrigin.y(), 
                 link->visualOrigin.z());
    
    // 应用RPY旋转 (Roll-Pitch-Yaw, ZYX顺序)
    glRotatef(qRadiansToDegrees(link->visualRPY.z()), 0, 0, 1);  // Yaw
    glRotatef(qRadiansToDegrees(link->visualRPY.y()), 0, 1, 0);  // Pitch
    glRotatef(qRadiansToDegrees(link->visualRPY.x()), 1, 0, 0);  // Roll
    
    // 渲染mesh
    glDrawElements(GL_TRIANGLES, buffer->indexCount, GL_UNSIGNED_INT, nullptr);
    
    glPopMatrix();
}
```

## 变换层次

现在完整的变换链是：

```
World坐标系
  └─ Joint origin变换 (parent link → child link)
      └─ Visual origin变换 (link → visual mesh)
          └─ STL网格顶点
```

对应的矩阵链：
```
最终位置 = WorldMatrix × JointMatrix × VisualOriginMatrix × VertexPosition
```

## 运行测试

```bash
cd /home/gxh/workspace/project/ActionPlanner
./ActionPlanner
```

点击"加载模型"，现在应该看到：
- ✅ STL模型正确连接
- ✅ 各个关节首尾相连
- ✅ 形成完整的机械臂结构

终端会输出每个link的visualOrigin：
```
URDFLoader: Added link: "left_j1_Link" visualOrigin: QVector3D(0, 0, 0)
URDFLoader: Added link: "left_j2_Link" visualOrigin: QVector3D(0.00883, 0.00179, -0.06471)
...
```
