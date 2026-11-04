# 世界坐标系方向调整

## 问题

URDF模型加载后是竖直的（Z轴向上），但实际应该横着放置。

## 解决方案

在`paintGL()`中添加世界坐标系旋转，**这是最干净的方式**：

```cpp
void RobotRenderer::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 设置相机
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(m_viewMatrix.constData());
    
    // 应用世界坐标系旋转 - 让机械臂横着
    glRotatef(90.0f, 1, 0, 0);  // 绕X轴旋转90度
    
    // 绘制坐标系和模型
    drawCoordinateFrame(0.3f);
    
    // 渲染所有link...
}
```

## 为什么选择世界坐标系旋转？

### ✅ 优点：
1. **不修改URDF数据** - URDF文件保持原样
2. **不改变相机逻辑** - 鼠标交互不受影响
3. **简单直接** - 一行代码搞定
4. **易于调整** - 改一个角度即可

### ❌ 其他方案的缺点：

**方案1: 修改相机初始角度**
```cpp
m_cameraRotationX = -90.0f;  // 不好
```
- 缺点：鼠标拖拽时会从奇怪的角度开始
- 缺点：坐标轴也会跟着歪

**方案2: 修改URDF文件**
```xml
<robot name="dualarm">
    <link name="world">
        <origin rpy="1.57 0 0" xyz="0 0 0"/>  <!-- 不好 -->
    </link>
</robot>
```
- 缺点：污染原始URDF数据
- 缺点：如果URDF文件被其他工具使用会有问题

**方案3: 修改每个link的变换**
```cpp
QMatrix4x4 worldRot;
worldRot.rotate(90, 1, 0, 0);
linkTransforms[linkName] = worldRot * transform;  // 不好
```
- 缺点：需要修改所有变换计算逻辑
- 缺点：代码更复杂

## 不同旋转方向

如果需要调整方向，修改`paintGL()`中的这一行：

```cpp
// 当前：向前横放（Y轴向前）
glRotatef(90.0f, 1, 0, 0);

// 向右横放（X轴向右）
glRotatef(90.0f, 0, 0, 1);

// 向左横放（X轴向左）
glRotatef(-90.0f, 0, 0, 1);

// 倒挂
glRotatef(180.0f, 1, 0, 0);

// 组合旋转：先绕X轴90度，再绕Z轴45度
glRotatef(90.0f, 1, 0, 0);
glRotatef(45.0f, 0, 0, 1);
```

## 测试

```bash
cd /home/gxh/workspace/project/ActionPlanner
./ActionPlanner
```

点击"加载模型"，现在机械臂应该横着显示了！

- 红色轴（X）：向右
- 绿色轴（Y）：向前
- 蓝色轴（Z）：向上
- 机械臂：沿着某个水平方向伸展

如果方向还不对，告诉我你希望机械臂朝哪个方向，我可以调整旋转角度。
