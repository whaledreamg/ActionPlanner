# 右臂控制问题修复总结

## 🔍 问题根源

推送消息的 `db` 字段中**没有 `name` 字段**，导致无法识别是左臂还是右臂的状态更新。

### 原始推送消息格式
```json
{
  "ty": "publish/robotState/sub",
  "topic": "robot/RightArm/status",
  "db": {
    "joints": [...],
    "enabled": true,
    // ❌ 没有 "name" 字段！
  }
}
```

### 问题表现

```
Robot name: ""  // ← 空的！
⚠️ Cannot identify arm, using as left: ""
```

所有消息都被误判为左臂，导致右臂控制无效。

---

## ✅ 修复方案

**从 `topic` 字段提取机器人名称**，然后添加到 `db` 中。

### 修复逻辑

```cpp
// topic格式: "robot/RightArm/status"
if (topic.startsWith("robot/") && topic.endsWith("/status")) {
    // 提取 "robot/" 和 "/status" 之间的部分
    QString robotName = topic.mid(6, topic.length() - 6 - 7);
    // robotName = "RightArm"
    
    // 将机器人名称添加到db中
    QJsonObject dbWithName = db;
    dbWithName["name"] = robotName;
    
    emit robotStateUpdated(dbWithName);
}
```

### 修改的文件

1. **`src/robot/RobotConnection.h`**:
   - 添加信号 `topicPushReceived(QString, QJsonObject)`

2. **`src/robot/RobotConnection.cpp`**:
   - 修改推送消息处理逻辑
   - 从 `topic` 提取机器人名称
   - 将名称注入到 `db` 中

---

## 🧪 测试

现在运行程序，应该看到：

```bash
cd /home/gxh/workspace/project/ActionPlanner
./ActionPlanner
```

### 预期日志

```
Received: {"ty":"publish/robotState/sub","topic":"robot/RightArm/status","db":{...}}
  -> Push message, topic: "robot/RightArm/status"
  -> Extracted robot name from topic: "RightArm"

=== onRobotStateUpdated called ===
State keys: QList("baseFrame", ..., "enabled", ..., "joints", ..., "name")  // ✓ 现在有 name 了！
Robot name: "RightArm"  // ✓ 不再是空的！
m_leftArmName: "LeftArm" , m_rightArmName: "RightArm"
Joints count: 7
✓ Matched right arm by name: RightArm  // ✓ 正确匹配！
✓ Updated RIGHT arm, emitted signal  // ✓ 更新右臂！
First 3 joints: QList(...)
Enabled status: true for RIGHT
=== onRobotStateUpdated finished ===
```

### 测试右臂使能

点击右臂"使能"按钮，应该看到：

```
========== onEnableRightClicked ==========
m_rightArmName: "RightArm"
Sending enable command to: "RightArm"
Sending: "{\"db\":{\"name\":\"RightArm\"},\"id\":16,\"ty\":\"robot/enable\"}"
Received: "{\"id\":16,\"ty\":\"robot/enable\",\"db\":true}"
  -> Response for id: 16 ty: "robot/enable"
```

---

## 📊 问题对比

### 修复前
- ❌ `Robot name: ""`（空）
- ❌ `Cannot identify arm, using as left`
- ❌ 所有状态更新都当作左臂
- ❌ 右臂按钮无效（因为 `m_rightArmName` 正确，但状态更新识别错误）

### 修复后
- ✅ `Robot name: "RightArm"`（从 topic 提取）
- ✅ `Matched right arm by name`
- ✅ 左右臂状态更新各自独立
- ✅ 右臂按钮工作正常

---

## 🔧 其他相关修复

1. **订阅格式**: `topics` 字段从数组改为字符串
2. **IP/端口默认值**: `192.168.100.16:9000`
3. **连接状态UI**: 绿色"已连接"显示
4. **详细调试日志**: 所有关键操作都有日志输出

---

## ✨ 现在应该可以了！

重新运行程序，左右臂应该都能正常工作：
- ✅ 状态实时更新（关节角度、使能状态）
- ✅ 使能/失能按钮正常
- ✅ Jog点动控制正常
- ✅ 获取当前角度正常

如果还有问题，把最新的日志发给我！ 🎯
