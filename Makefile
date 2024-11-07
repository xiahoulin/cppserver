# 编译器设置
CXX = g++
CXXFLAGS = -std=c++11 -Wall -I./include -I/usr/include

# 系统检测
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    CXXFLAGS += $(shell pkg-config --cflags jsoncpp)
endif

# 依赖库
LIBS = -lsqlite3 -ljsoncpp

# 源文件目录和目标文件目录
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# 源文件和目标文件
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# 可执行文件
TARGET = $(BIN_DIR)/server

# 默认目标
all: directories $(TARGET)

# 创建必要的目录
directories:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)
	@mkdir -p db

# 编译可执行文件
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LIBS)

# 编译源文件
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean directories 