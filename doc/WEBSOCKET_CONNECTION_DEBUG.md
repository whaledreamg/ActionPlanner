# WebSocket连接诊断

## 当前问题

连接 `ws://192.168.100.16:9001` 失败

## 诊断结果

### ✅ 网络连通性
```bash
$ ping 192.168.100.16
64 bytes from 192.168.100.16: icmp_seq=1 ttl=64 time=3.26 ms
✓ 网络连通正常
```

### ✅ 端口开放
```bash
$ nc -zv 192.168.100.16 9001
Connection to 192.168.100.16 9001 port [tcp/*] succeeded!
✓ 端口9001开放
```

### ⚠️ WebSocket握手异常
```bash
$ curl -i --no-buffer -H "Connection: Upgrade" -H "Upgrade: websocket" ...
curl: (1) Received HTTP/0.9 when not allowed
⚠️ 服务器响应格式不标准（可能是原始TCP或非标准WebSocket）
```

## 可能原因

### 1. 服务器地址或端口错误
机械臂控制器的WebSocket端口可能不是9001。请检查：
- 默认端口通常是 **9000** 或 **8080**
- 查看控制器文档或配置文件

### 2. 服务器未启动WebSocket服务
- 控制器软件可能没有运行
- WebSocket服务可能被禁用

### 3. 防火墙阻拦
```bash
# 检查防火墙（在控制器端执行）
sudo iptables -L -n | grep 9001
sudo ufw status
```

### 4. 需要特殊的WebSocket子协议
某些服务器要求特定的子协议头，例如：
```cpp
QWebSocket *socket = new QWebSocket();
socket->open(QUrl("ws://192.168.100.16:9001"), 
             QStringLiteral("robot-protocol"));  // 子协议
```

## 调试步骤

### 步骤1：确认正确的端口

**查看参考实现的连接配置**：
```bash
grep -r "9000\|9001\|8080" /home/gxh/workspace/project/RobotControl/
```

或查看控制器文档中的WebSocket端口。

### 步骤2：使用测试工具验证

我已经添加了详细的调试输出。运行程序后，你会看到：

**连接成功**：
```
==========================================
Connecting to WebSocket: ws://192.168.100.16:9001
Host: 192.168.100.16 Port: 9001
Opening WebSocket connection...
Connection initiated, state: 2  (Connecting)
==========================================
✓ WebSocket connected successfully!
✓ State: 3  (Connected)
==========================================
```

**连接失败**：
```
==========================================
Connecting to WebSocket: ws://192.168.100.16:9001
Host: 192.168.100.16 Port: 9001
Opening WebSocket connection...
Connection initiated, state: 2  (Connecting)
==========================================
✗ WebSocket error: Connection refused
✗ Error code: 0
✗ State: 0  (Unconnected)
==========================================
```

或者：
```
==========================================
✗ WebSocket disconnected
✗ Close code: 1006  (Abnormal Disconnection)
✗ Close reason: 
==========================================
```

### 步骤3：使用Python测试（参考实现的工具）

```bash
cd /home/gxh/workspace/project/RobotControl/communication
python3
>>> from arm_websocket import ArmWebSocket
>>> ws = ArmWebSocket()
>>> ws.connect("192.168.100.16", 9001)
# 查看是否成功
```

### 步骤4：检查控制器状态

在控制器端：
```bash
# 检查WebSocket服务是否运行
netstat -tuln | grep 9001
ps aux | grep robot

# 查看控制器日志
tail -f /var/log/robot-controller.log
```

## 快速测试命令

```bash
# 1. 测试不同端口
./ActionPlanner  # 在UI中试试 9000 端口

# 2. 使用websocat工具测试（如果有）
websocat ws://192.168.100.16:9001

# 3. 使用Python快速测试
python3 -c "
import websocket
ws = websocket.WebSocket()
try:
    ws.connect('ws://192.168.100.16:9001')
    print('✓ Connected successfully')
    ws.close()
except Exception as e:
    print(f'✗ Failed: {e}')
"
```

## 常见端口配置

| 服务 | 默认端口 | 说明 |
|------|----------|------|
| WebSocket (标准) | 9000 | 最常见 |
| WebSocket (备用) | 9001 | 备用端口 |
| HTTP API | 8080 | REST API |
| 调试端口 | 8081 | 诊断工具 |

## 下一步

1. **确认端口号** - 检查控制器文档或参考实现的配置
2. **运行程序看详细错误** - 我已经添加了完整的错误输出
3. **如果端口对但连不上** - 检查控制器服务是否启动
4. **提供完整错误日志** - 把终端输出发给我，包括错误代码和状态

## 测试

```bash
cd /home/gxh/workspace/project/ActionPlanner
./ActionPlanner
```

点击连接按钮，查看终端输出的详细错误信息，然后告诉我具体的错误内容！
