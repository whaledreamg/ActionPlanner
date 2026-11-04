# 右臂控制调试指南

## 问题描述

右臂控制没有反应（无法使能/失能、数据不更新）

## 可能原因

### 1. 机器人名称未正确识别

最可能的原因是 `m_rightArmName` 为空或不正确。

### 2. 订阅消息未发送/失败

订阅格式不正确或服务器拒绝订阅。

### 3. 推送消息未正确解析

服务器推送的右臂消息没有被正确路由到 `onRobotStateUpdated`。

## 调试步骤

### 第1步：检查机器人列表

连接成功后，终端应该显示：

```
========== onRobotListReceived ==========
Robots received: [{"name":"LeftArm",...}, {"name":"RightArm",...}]
✓ Left arm name: LeftArm
✓ Right arm name: RightArm
========================================
```

**重点检查**：
- `robots.size()` 是否 >= 2？
- 如果只有1个机器人，会显示什么？
- `m_rightArmName` 最终是什么值？

### 第2步：检查订阅消息

应该看到两条订阅消息：

```
Subscribing to LeftArm status with tc = 50 ms
Sending subscription: {"id":2,"tc":50,"topics":"robot/LeftArm/status","ty":"robotState/sub"}

Subscribing to RightArm status with tc = 50 ms
Sending subscription: {"id":3,"tc":50,"topics":"robot/RightArm/status","ty":"robotState/sub"}
```

**重点检查**：
- 是否发送了两条订阅？
- `topics` 字段是**字符串**还是数组？（应该是字符串！）
- 右臂的机器人名称是否正确？

### 第3步：检查推送消息

服务器应该推送右臂状态：

```
Received: {"ty":"publish/robotState/sub","topic":"robot/RightArm/status","db":{...}}

=== onRobotStateUpdated called ===
State keys: ("name", "joints", "cartesian", "enabled")
Robot name: RightArm
m_leftArmName: LeftArm , m_rightArmName: RightArm
Joints count: 7
✓ Matched right arm by name: RightArm
✓ Updated RIGHT arm, emitted signal
First 3 joints: [0.1, 0.2, 0.3]
=== onRobotStateUpdated finished ===
```

**重点检查**：
- 是否收到了 `robot/RightArm/status` 的推送？
- `Robot name` 是什么？
- 是否匹配到右臂？（应该看到 "Matched right arm by name"）
- 是否 `Updated RIGHT arm`？

### 第4步：检查使能按钮

点击右臂使能/失能按钮，应该显示：

```
========== onEnableRightClicked ==========
m_rightArmName: RightArm
Sending enable command to: RightArm
```

**重点检查**：
- `m_rightArmName` 是否为空？
- 是否看到 "Right arm name is empty!" 警告？

## 常见问题

### 问题1: `m_rightArmName` 为空

**原因**: `robot/list` 只返回了1个机器人，且被识别为左臂。

**解决**: 
- 检查控制器是否连接了两个机械臂
- 如果只有1个右臂，修改识别逻辑

### 问题2: 订阅消息格式错误

**原因**: `topics` 是数组而不是字符串。

**已修复**: 现在使用字符串格式：
```cpp
sub["topics"] = QString("robot/%1/status").arg(robotName);
```

### 问题3: 机器人名称大小写不匹配

**原因**: 服务器返回 "rightArm"，但代码期望 "RightArm"。

**解决**: 使用不区分大小写的匹配（已实现）。

### 问题4: 右臂标签未创建

**原因**: `m_rightJointLabels` 为空。

**检查**: `createJointControls()` 是否正确创建了右臂控件？

## 完整测试流程

```bash
cd /home/gxh/workspace/project/ActionPlanner
./ActionPlanner 2>&1 | tee debug_right_arm.log
```

1. **连接**：
   - 输入 IP: `192.168.100.16`
   - 端口: `9000`
   - 点击"连接机器人"

2. **观察日志**：
   - 找到 "onRobotListReceived" 部分
   - 找到两条 "Sending subscription" 消息
   - 找到 "onRobotStateUpdated" 消息（应该有左右臂各一份）

3. **测试右臂**：
   - 切换到"右臂控制"标签页
   - 观察关节角度是否更新（J1~J7）
   - 点击"使能"按钮
   - 观察终端是否显示 "Sending enable command to: RightArm"

4. **如果右臂还是没反应**：
   - 把完整的日志文件 `debug_right_arm.log` 发给我
   - 特别是这几个部分：
     - `onRobotListReceived`
     - `Sending subscription`
     - `onRobotStateUpdated`（所有的，包括左右臂）
     - `onEnableRightClicked`

## 快速诊断命令

```bash
# 只看关键日志
./ActionPlanner 2>&1 | grep -E "onRobotListReceived|m_.*ArmName|Sending subscription|Robot name:|Matched.*arm|onEnable.*Clicked"
```

## 预期输出模板

```
========== onRobotListReceived ==========
Robots received: [...]
✓ Left arm name: LeftArm
✓ Right arm name: RightArm
========================================

Subscribing to LeftArm status with tc = 50 ms
Sending subscription: {"id":2,"tc":50,"topics":"robot/LeftArm/status","ty":"robotState/sub"}

Subscribing to RightArm status with tc = 50 ms
Sending subscription: {"id":3,"tc":50,"topics":"robot/RightArm/status","ty":"robotState/sub"}

=== onRobotStateUpdated called ===
Robot name: LeftArm
m_leftArmName: LeftArm , m_rightArmName: RightArm
✓ Matched left arm by name: LeftArm
✓ Updated LEFT arm, emitted signal

=== onRobotStateUpdated called ===
Robot name: RightArm
m_leftArmName: LeftArm , m_rightArmName: RightArm
✓ Matched right arm by name: RightArm
✓ Updated RIGHT arm, emitted signal

========== onEnableRightClicked ==========
m_rightArmName: RightArm
Sending enable command to: RightArm
```

---

**现在运行程序，把关键日志发给我！** 🔍
