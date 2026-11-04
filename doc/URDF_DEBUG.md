# URDF加载问题排查指南

## 问题诊断步骤

### 1. 检查URDF文件
```bash
cd /home/gxh/workspace/project/ActionPlanner
ls -lh urdf_dualarm/*.urdf
```

已找到: `urdf_dualarm/dualarm.urdf` (17KB)

### 2. 运行程序并查看调试输出
程序已增加调试日志,启动时会输出:
- URDF文件扫描结果
- 加载过程详情
- Links和Joints数量

```bash
cd /home/gxh/workspace/project/ActionPlanner
./ActionPlanner
```

在终端中查看类似以下的调试输出:
```
SimulationWidget initialized, renderer created
Found X URDF files in urdf_dualarm
  - dualarm.urdf
Loading URDF from: /path/to/dualarm.urdf
loadURDF called with: /path/to/dualarm.urdf
URDF loaded successfully
Links: XX
Joints: XX
RobotRenderer::setURDFData - Links: XX Joints: XX
Drawing XX joints
```

### 3. 可能的问题

#### 问题A: URDF文件未找到
- 确认`urdf_dualarm`文件夹与可执行文件在同一目录
- 确认URDF文件权限可读

#### 问题B: URDF解析失败
- 检查URDF XML格式是否正确
- 查看是否有tinyxml2解析错误

#### 问题C: OpenGL渲染问题
- 3D窗口是否正确创建(应该看到坐标轴)
- 鼠标拖动和滚轮缩放是否正常
- 检查OpenGL驱动是否正常

### 4. 手动测试步骤

1. **启动程序**
   ```bash
   cd /home/gxh/workspace/project/ActionPlanner
   ./ActionPlanner 2>&1 | tee debug.log
   ```

2. **在3D仿真标签页**:
   - 下拉框应显示`dualarm.urdf`
   - 点击"加载模型"按钮
   - 查看终端调试输出

3. **预期结果**:
   - 应在3D窗口中看到红/绿/蓝坐标轴
   - 应看到机器人关节球体(灰色)
   - 应看到关节间连杆(圆柱)
   - 可以用鼠标左键拖动旋转视角
   - 可以用滚轮缩放

### 5. 快速修复方案

如果仍然看不到模型:

1. 确认RobotRenderer已正确嵌入UI:
   - 检查`renderContainer`是否可见
   - 检查OpenGL上下文是否初始化

2. 检查URDF文件路径:
   - 使用绝对路径浏览按钮加载
   - 确认文件确实被解析

3. 增加更多调试输出来定位问题

### 当前代码改进

已添加的调试输出点:
- `SimulationWidget::scanURDFFiles()` - 扫描结果
- `SimulationWidget::onLoadClicked()` - 加载触发
- `SimulationWidget::loadURDF()` - 加载状态
- `URDFLoader::loadURDF()` - 解析过程(已有qWarning)
- `RobotRenderer::setURDFData()` - 数据接收
- `RobotRenderer::paintGL()` - 绘制过程

重新编译后运行程序,观察终端输出即可诊断问题所在。
