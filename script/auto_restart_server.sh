#!/bin/bash

# 输出脚本自身的进程ID
echo "脚本进程ID: $$"

# 定义可执行文件路径
EXECUTABLE_PATH="/tmp/data/AresServer/cmake-build-debug-ubuntu_aresserver/Server14"
CONFIG_PATH="/tmp/data/config.ini"
LOAD_FILE_PATH="/tmp/data/material/"
RES_FILE_PATH="/tmp/data/result/"
# 信号处理函数
function cleanup {
    echo "接收到终止信号，正在关闭服务器..."
    # 查找并杀死服务器进程
    pkill -f "$EXECUTABLE_PATH"
    exit 0
}

# 注册终止信号处理程序
trap cleanup SIGINT SIGTERM

# 检查是否具有 root 权限
if [ "$(id -u)" -ne 0 ]; then
  echo "请以 root 权限运行此脚本。"
  exit 1
fi


# 启动可执行文件并输出其进程ID
echo "启动服务器..."
$EXECUTABLE_PATH "$CONFIG_PATH" &
echo "可执行文件进程ID: $!"

# 进入循环，等待信号
while true; do
  sleep 3000  # 5分钟
  echo "检查并重启服务器..."
  # 查找并杀死服务器进程
  pkill -f "$EXECUTABLE_PATH"

  # 获取所有匹配的进程ID
  pids=$(pgrep -f $EXECUTABLE_PATH)

  # 判断是否存在进程ID，如果存在则逐一杀死进程
  if [ -n "$pids" ]; then
      echo "正在杀死进程: $pids"
      for pid in $pids; do
          echo "杀死进程 $pid"
          kill -9 "$pid"
      done
  else
      echo "死光了"
  fi
  rm -rf "$LOAD_FILE_PATH"*
  rm -rf "$RES_FILE_PATH"*

  # 重新启动服务器并输出其进程ID

  sleep 2
  # 先释放端口 80
  sudo lsof -t -i:80 | xargs kill -9
  sleep 3

  $EXECUTABLE_PATH "$CONFIG_PATH" &
  echo "可执行文件进程ID: $!"
done
