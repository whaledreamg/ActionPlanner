# ActionPlanner - Qt6 编译指南

## 环境要求

- Qt 6.8.3 (或更高版本)
- GCC 11+
- TinyXML2
- OpenGL开发库
- Eigen3 (可选)

## Qt6 编译

### 方法1: 使用绝对路径的qmake

```bash
cd /home/gxh/workspace/project/ActionPlanner
make clean
/home/gxh/Qt/6.8.3/gcc_64/bin/qmake
make -j4
```

### 方法2: 将Qt6添加到PATH

```bash
export PATH=/home/gxh/Qt/6.8.3/gcc_64/bin:$PATH
export LD_LIBRARY_PATH=/home/gxh/Qt/6.8.3/gcc_64/lib:$LD_LIBRARY_PATH

cd /home/gxh/workspace/project/ActionPlanner
make clean
qmake
make -j4
```

## 运行程序

### 需要设置Qt6库路径

```bash
export LD_LIBRARY_PATH=/home/gxh/Qt/6.8.3/gcc_64/lib:$LD_LIBRARY_PATH
./ActionPlanner
```

### 或者一行命令运行

```bash
LD_LIBRARY_PATH=/home/gxh/Qt/6.8.3/gcc_64/lib:$LD_LIBRARY_PATH ./ActionPlanner
```

## Qt5 vs Qt6 差异

项目已适配Qt6,主要变化:

1. **QOpenGLWidget 模块**: Qt6需要显式添加`openglwidgets`模块
2. **Socket错误信号**: `error()` 改为 `errorOccurred()`
3. **鼠标事件**: `event->x()/y()` 改为 `event->position().x()/y()`

代码中已通过条件编译兼容Qt5/Qt6:

```cpp
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_webSocket, &QAbstractSocket::errorOccurred,
            this, &RobotConnection::onError);
#else
    connect(m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, &RobotConnection::onError);
#endif
```

## Qt版本检测

查看当前使用的qmake版本:

```bash
qmake --version
# 或
/home/gxh/Qt/6.8.3/gcc_64/bin/qmake --version
```

输出应该显示: `Using Qt version 6.8.3 ...`

## 已启用的Qt模块

Qt6编译时启用了以下模块:
- ✅ core, gui, widgets
- ✅ network (TCP socket)
- ✅ websockets (WebSocket支持)
- ✅ openglwidgets (OpenGL 3D渲染)
- ✅ 3dcore, 3drender, 3dinput (Qt3D模块)

## 已知警告

编译时有以下警告,不影响功能:

1. **QMouseEvent deprecated API**: 
   - `event->x()/y()` 应改为 `event->position().x()/y()`
   - 影响: `RobotRenderer::mouseMoveEvent()`

2. **unused parameter 'error'**:
   - `RobotConnection::onError()` 的参数未使用
   - 可以用 `Q_UNUSED(error)` 或 `[[maybe_unused]]` 标注

## 依赖安装 (Ubuntu/Debian)

```bash
# TinyXML2
sudo apt install libtinyxml2-dev

# OpenGL
sudo apt install libgl1-mesa-dev libglu1-mesa-dev

# Eigen3 (可选)
sudo apt install libeigen3-dev
```

## 常见问题

### Q: 找不到 QOpenGLWidget

**A**: 确认.pro文件包含:
```qmake
greaterThan(QT_MAJOR_VERSION, 5) {
    QT += openglwidgets
}
```

### Q: 运行时提示找不到Qt库

**A**: 设置LD_LIBRARY_PATH:
```bash
export LD_LIBRARY_PATH=/home/gxh/Qt/6.8.3/gcc_64/lib:$LD_LIBRARY_PATH
```

### Q: 想用Qt5编译

**A**: 使用系统默认qmake:
```bash
qmake  # 使用 /usr/bin/qmake (Qt5)
make -j4
```

项目代码兼容Qt5.15+和Qt6。

## 性能提示

- Qt6在OpenGL渲染方面性能更好
- Qt6支持更现代的C++17/20特性
- 建议生产环境使用Qt6.2 LTS或更高版本

---

**编译状态**: ✅ 成功 (Qt 6.8.3)  
**最后更新**: 2026-11-02
