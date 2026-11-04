#!/bin/bash
# 启动ActionPlanner并自动显示日志

echo "=========================================="
echo "启动 ActionPlanner"
echo "连接: ws://192.168.100.16:9000"
echo "=========================================="
echo ""
echo "请在UI中："
echo "  1. 输入IP: 192.168.100.16"
echo "  2. 端口默认: 9000"
echo "  3. 点击'连接'按钮"
echo ""
echo "观察下面的日志输出..."
echo "=========================================="
echo ""

cd /home/gxh/workspace/project/ActionPlanner
./ActionPlanner 2>&1 | while IFS= read -r line; do
    # 高亮重要信息
    if echo "$line" | grep -q "WebSocket connected"; then
        echo -e "\e[32m✓ $line\e[0m"
    elif echo "$line" | grep -q "error\|Error\|ERROR\|✗"; then
        echo -e "\e[31m✗ $line\e[0m"
    elif echo "$line" | grep -q "Connecting\|Subscribing"; then
        echo -e "\e[33m→ $line\e[0m"
    elif echo "$line" | grep -q "onRobotStateUpdated"; then
        echo -e "\e[36m★ $line\e[0m"
    else
        echo "$line"
    fi
done
