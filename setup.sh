#!/bin/bash

# 设置错误时退出
set -e

echo "开始安装依赖..."

# 检测系统类型
if [ -f /etc/debian_version ]; then
    # Debian/Ubuntu系统
    echo "检测到Debian/Ubuntu系统"
    sudo apt-get update
    sudo apt-get install -y build-essential pkg-config
    sudo apt-get install -y sqlite3 libsqlite3-dev
    sudo apt-get install -y libjsoncpp-dev
    
    # 检查jsoncpp头文件
    if [ ! -f "/usr/include/jsoncpp/json/json.h" ]; then
        echo "创建jsoncpp头文件软链接..."
        sudo ln -sf /usr/include/jsoncpp/json /usr/include/json
    fi

elif [ -f /etc/redhat-release ]; then
    # CentOS/RHEL系统
    echo "检测到CentOS/RHEL系统"
    sudo yum update
    sudo yum groupinstall -y "Development Tools"
    sudo yum install -y sqlite-devel
    sudo yum install -y jsoncpp-devel
    
    # 检查jsoncpp头文件
    if [ ! -f "/usr/include/jsoncpp/json/json.h" ]; then
        echo "创建jsoncpp头文件软链接..."
        sudo ln -sf /usr/include/jsoncpp/json /usr/include/json
    fi
else
    echo "不支持的系统类型"
    exit 1
fi

echo "依赖安装完成"

# 创建数据库目录
echo "创建数据库..."
mkdir -p db
sqlite3 ./db/database.db << EOF
CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL,
    email TEXT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
EOF

echo "数据库创建完成"

# 检查pkg-config是否正确配置
echo "检查pkg-config配置..."
pkg-config --cflags jsoncpp || {
    echo "警告: pkg-config未找到jsoncpp配置"
    echo "尝试使用默认路径..."
}

# 编译项目
echo "开始编译项目..."
make clean
make

echo "编译完成！"
echo "服务器程序位于 bin/server"