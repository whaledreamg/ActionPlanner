# WebSocket连接指南

## ✅ 连接参数确认

- **IP地址**: `192.168.100.16`
- **端口**: `9000` (默认)
- **WebSocket URL**: `ws://192.168.100.16:9000`

## ✅ 测试结果

```bash
✓ 网络连通
✓ 端口9000开放
✓ Python WebSocket连接成功
```

**说明**: 服务器WebSocket服务工作正常！

## 🚀 启动程序

### 方法1: 直接启动
```bash
cd /home/gxh/workspace/project/ActionPlanner
./ActionPlanner
```

### 方法2: 带调试日志启动（推荐）
```bash
cd /home/gxh/workspace/project/ActionPlanner
./run_with_debug.sh
```

## 📝 操作步骤

1. **启动程序**
2. **在UI中输入**:
   - IP地址: `192.168.100.16`
   - 端口: `9000` (默认，可以不填)
3. **点击"连接"按钮**
4. **观察终端输出**

## 📊 预期输出

### ✅ 连接成功
```
==========================================
Connecting to WebSocket: ws://192.168.100.16:9000
Host: 192.168.100.16 Port: 9000
Opening WebSocket connection...
Connection initiated, state: 2  (Connecting)
==========================================
✓ WebSocket connected successfully!
✓ State: 3  (Connected)
==========================================
Starting subscriptions...
Subscribing to robot states...
Subscribing to LeftArm status with tc = 50 ms
Sending subscription: {"id":2,"tc":50,"topics":["robot/LeftArm/status"],"ty":"robotState/sub"}
Subscribing to RightArm status with tc = 50 ms
Sending subscription: {"id":3,"tc":50,"topics":["robot/RightArm/status"],"ty":"robotState/sub"}
Fetching initial status...
Received: {"ty":"publish/robotState/sub","topic":"robot/LeftArm/status","db":{...}}
=== onRobotStateUpdated called ===
State keys: ("name", "joints", "cartesian", "enabled")
Robot name: LeftArm
Joints count: 7
Is left arm: true
Updated LEFT arm, emitted signal
First 3 joints: [0.1, 0.2, 0.3]
=== onRobotStateUpdated finished ===
```

### ❌ 如果连接失败
```
==========================================
✗ WebSocket error: Connection refused
✗ Error code: 2
✗ State: 0  (Unconnected)
==========================================
```

**解决方案**:
- 检查控制器服务是否启动
- 确认IP地址正确
- 确认防火墙没有阻拦

## 🎯 UI更新

连接成功后，UI应该显示：
- ✅ 左右臂关节角度实时更新（每50ms）
- ✅ 使能/失能按钮状态正确
- ✅ 3D仿真跟随运动
- ✅ TCP位置显示

## 🔧 调试技巧

### 查看完整日志
```bash
cd /home/gxh/workspace/project/ActionPlanner
./ActionPlanner 2>&1 | tee connection.log
```

### 只看WebSocket相关
```bash
./ActionPlanner 2>&1 | grep -E "WebSocket|Connecting|Subscribing|onRobotState"
```

### 只看错误
```bash
./ActionPlanner 2>&1 | grep -E "error|Error|ERROR|✗"
```

## 📞 如果仍有问题

请提供完整的终端输出（从"Connecting to WebSocket"到错误信息），我会帮你诊断！

---

**现在就试试吧！** 🚀

```bash
cd /home/gxh/workspace/project/ActionPlanner
./ActionPlanner
```
