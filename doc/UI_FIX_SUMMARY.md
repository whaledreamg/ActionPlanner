# UI修复总结

## 已修复的问题

### 1. ✅ IP和端口默认值更新
- **IP**: `192.168.1.100` → `192.168.100.16`
- **端口**: `9001` → `9000`

### 2. ✅ 连接状态UI更新
**未连接状态**:
- 按钮文本: "连接机器人"
- 状态标签: "● 未连接" (灰色)
- IP/端口输入框: 可编辑

**连接成功状态**:
- 按钮文本: "✓ 已连接 (点击断开)"
- 按钮颜色: 绿色 (#6EE7B7)
- 状态标签: "● 已连接" (绿色, 加粗)
- IP/端口输入框: 锁定不可编辑
- 使能/失能按钮: 启用

### 3. ✅ 左臂识别增强
增加了多种机器人名称识别方式：
1. **完全匹配**: `robotName == m_leftArmName`
2. **关键字匹配**: 包含"Left", "left", "L"
3. **完全匹配右臂**: `robotName == m_rightArmName`
4. **关键字匹配**: 包含"Right", "right", "R"
5. **详细日志**: 打印识别过程

### 4. ✅ 机器人列表处理增强
- 支持2个机器人（双臂）
- 支持1个机器人（根据名称自动判断）
- 详细的调试输出

## 调试输出

现在会看到完整的日志：

```
========== onRobotListReceived ==========
Robots received: [{"name":"LeftArm",...}, {"name":"RightArm",...}]
✓ Left arm name: LeftArm
✓ Right arm name: RightArm
========================================

=== onRobotStateUpdated called ===
State keys: ("name", "joints", "cartesian", "enabled")
Robot name: LeftArm
m_leftArmName: LeftArm , m_rightArmName: RightArm
Joints count: 7
✓ Matched left arm by name: LeftArm
✓ Updated LEFT arm, emitted signal
First 3 joints: [0.1, 0.2, 0.3]
=== onRobotStateUpdated finished ===
```

## 可能的左臂问题原因

如果左臂还是没反应，可能是：

### 1. 机器人名称不匹配
查看日志中的：
```
Robot name: XXXXX
m_leftArmName: YYYYY , m_rightArmName: ZZZZZ
```

如果`XXXXX`既不等于`YYYYY`，也不包含"left"关键字，就会被误判为右臂。

### 2. 订阅失败
检查是否有订阅消息：
```
Subscribing to LeftArm status with tc = 50 ms
Sending subscription: {"id":2,"tc":50,"topics":["robot/LeftArm/status"],"ty":"robotState/sub"}
```

### 3. 服务器只推送一个臂
可能控制器只连接了一个机械臂，或者只有一个臂在推送状态。

## 测试步骤

```bash
cd /home/gxh/workspace/project/ActionPlanner
./ActionPlanner
```

1. 确认IP显示 `192.168.100.16`
2. 确认端口显示 `9000`
3. 点击"连接机器人"
4. 观察按钮变为"✓ 已连接 (点击断开)"，绿色
5. 观察状态变为"● 已连接"，绿色
6. 查看终端日志，找到机器人名称
7. 把完整日志发给我

## 需要的日志信息

重点关注这些输出：
```
========== onRobotListReceived ==========
Robots received: [...]
✓ Left arm name: ???
✓ Right arm name: ???

=== onRobotStateUpdated called ===
Robot name: ???
m_leftArmName: ??? , m_rightArmName: ???
✓ Matched ??? arm by ???
```

**把这些日志发给我，我会帮你诊断左臂问题！** 🔍
