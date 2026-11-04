#!/bin/bash
# WebSocket连接测试脚本

echo "=========================================="
echo "WebSocket连接测试"
echo "目标: ws://192.168.100.16:9000"
echo "=========================================="
echo ""

# 1. 网络测试
echo "1️⃣ 测试网络连通性..."
if ping -c 1 -W 2 192.168.100.16 > /dev/null 2>&1; then
    echo "   ✓ 网络连通"
else
    echo "   ✗ 网络不通"
    exit 1
fi
echo ""

# 2. 端口测试
echo "2️⃣ 测试端口9000..."
if nc -zv -w 2 192.168.100.16 9000 2>&1 | grep -q succeeded; then
    echo "   ✓ 端口开放"
else
    echo "   ✗ 端口关闭"
    exit 1
fi
echo ""

# 3. WebSocket握手测试
echo "3️⃣ 测试WebSocket握手..."
response=$(timeout 2 curl -i --no-buffer \
    -H "Connection: Upgrade" \
    -H "Upgrade: websocket" \
    -H "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==" \
    -H "Sec-WebSocket-Version: 13" \
    http://192.168.100.16:9000/ 2>&1)

if echo "$response" | grep -qi "101\|switching protocols"; then
    echo "   ✓ WebSocket握手成功"
elif echo "$response" | grep -qi "HTTP"; then
    echo "   ⚠️ 服务器响应HTTP但不是101"
    echo "   响应: $(echo "$response" | head -3)"
else
    echo "   ⚠️ 非标准响应"
    echo "   $(echo "$response" | head -3)"
fi
echo ""

echo "=========================================="
echo "✓ 基础连接测试完成"
echo "现在运行程序测试 Qt WebSocket 连接："
echo "  cd /home/gxh/workspace/project/ActionPlanner"
echo "  ./ActionPlanner"
echo "=========================================="
