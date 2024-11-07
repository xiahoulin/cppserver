#include "epoll_server.h"
#include "sqlite_connect_handler.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <json/json.h>
#include <iostream>
#include <cstring>
#include <errno.h>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <chrono>

EpollServer::EpollServer(int port)
    : serverFd(-1), epollFd(-1)
{
    // 创建服务器socket
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    // 绑定地址并开始监听
    bind(serverFd, (struct sockaddr*)&addr, sizeof(addr));
    listen(serverFd, SOMAXCONN);
    
    // 创建epoll实例
    epollFd = epoll_create1(0);
}

EpollServer::~EpollServer() {
    if (serverFd >= 0) {
        close(serverFd);
    }
    if (epollFd >= 0) {
        close(epollFd);
    }
}

void EpollServer::start() {
    struct epoll_event ev, events[10];
    
    // 设置服务器socket为非阻塞
    int flags = fcntl(serverFd, F_GETFL, 0);
    fcntl(serverFd, F_SETFL, flags | O_NONBLOCK);
    
    // 添加服务器socket到epoll
    ev.events = EPOLLIN;
    ev.data.fd = serverFd;
    epoll_ctl(epollFd, EPOLL_CTL_ADD, serverFd, &ev);
    
    std::cout << "服务器启动，等待连接..." << std::endl;
    
    while (true) {
        int nfds = epoll_wait(epollFd, events, 10, -1);
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == serverFd) {
                // 新的客户端连接
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int clientFd = accept(serverFd, (struct sockaddr*)&client_addr, &client_len);
                
                if (clientFd < 0) {
                    std::cerr << "接受连接失败: " << std::strerror(errno) << std::endl;
                    continue;
                }
                
                // 设置客户端socket为非阻塞
                flags = fcntl(clientFd, F_GETFL, 0);
                fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);
                
                // 添加到epoll
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = clientFd;
                epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &ev);
                
                std::cout << "新客户端连接，fd = " << clientFd << std::endl;
            } else {
                // 处理客户端请求
                handleConnection(events[i].data.fd);
            }
        }
    }
}

void EpollServer::handleConnection(int clientFd) {
    char buffer[1024];
    ssize_t n = read(clientFd, buffer, sizeof(buffer)-1);
    
    if (n <= 0) {
        if (n < 0) {
            logError(getClientInfo(clientFd) + " Read error: " + std::strerror(errno));
        }
        closeConnection(clientFd);
        return;
    }
    
    buffer[n] = '\0';
    std::string response = processRequest(buffer, clientFd);
    write(clientFd, response.c_str(), response.length());
}

void EpollServer::registerHandler(const std::string& funcId, 
                                std::function<std::string(const Json::Value&, int)> handler) {
    handlers[funcId] = handler;
}

std::string EpollServer::processRequest(const std::string& request, int clientFd) {
    Json::Value root;
    Json::Reader reader;
    logDebug("Received request: " + request);

    if (!reader.parse(request, root)) {
        return "{\"status\":-1,\"msg\":\"Invalid JSON format\"}";
    }
    
    std::string funcId = root["funcid"].asString();
    auto it = handlers.find(funcId);
    if (it == handlers.end()) {
        return "{\"status\":-1,\"msg\":\"Unknown funcid\"}";
    }
    
    return it->second(root, clientFd);
}

// 添加日志辅助方法的实现
void EpollServer::logDebug(const std::string& message) const {
    std::cout << "[DEBUG][" << getCurrentTimestamp() << "] " << message << std::endl;
}

void EpollServer::logError(const std::string& message) const {
    std::cerr << "[ERROR][" << getCurrentTimestamp() << "] " << message << std::endl;
}

std::string EpollServer::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    time_t now_time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    char buffer[32];
    struct tm* timeinfo = localtime(&now_time);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    std::stringstream ss;
    ss << buffer << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string EpollServer::getClientInfo(int clientFd) const {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    char ip[INET_ADDRSTRLEN];
    
    if (getpeername(clientFd, (struct sockaddr*)&addr, &addr_len) < 0) {
        return "Client[" + std::to_string(clientFd) + ": Unknown]";
    }
    
    if (inet_ntop(AF_INET, &(addr.sin_addr), ip, INET_ADDRSTRLEN) == nullptr) {
        return "Client[" + std::to_string(clientFd) + ": Invalid IP]";
    }
    
    std::stringstream ss;
    ss << "Client[" << clientFd << ": " << ip << ":" << ntohs(addr.sin_port) << "]";
    return ss.str();
}

// 在handleAccept方法中添加日志
void EpollServer::handleAccept() {
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    
    int clientFd = accept(listenFd, (struct sockaddr*)&clientAddr, &clientAddrLen);
    if (clientFd < 0) {
        logError("Accept failed: " + std::string(strerror(errno)));
        return;
    }
    
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), ip, INET_ADDRSTRLEN);
    logDebug("New connection accepted: " + std::string(ip) + ":" + 
             std::to_string(ntohs(clientAddr.sin_port)) + " (fd: " + 
             std::to_string(clientFd) + ")");
    
    // ... 其余accept逻辑 ...
}

// 在handleRead方法中添加日志
void EpollServer::handleRead(int fd) {
    char buffer[4096];
    ssize_t n;
    
    logDebug(getClientInfo(fd) + " Reading data...");
    
    n = read(fd, buffer, sizeof(buffer));
    if (n < 0) {
        logError(getClientInfo(fd) + " Read error: " + std::string(strerror(errno)));
        closeConnection(fd);
        return;
    }
    
    if (n == 0) {
        logDebug(getClientInfo(fd) + " Connection closed by client");
        closeConnection(fd);
        return;
    }
    
    buffer[n] = '\0';
    logDebug(getClientInfo(fd) + " Received " + std::to_string(n) + " bytes: " + 
             std::string(buffer));
    
    // ... 处理接收到的数据 ...
}

// 在handleWrite方法中添加日志
void EpollServer::handleWrite(int fd) {
    logDebug(getClientInfo(fd) + " Writing response...");
    
    // 获取待发送的数据
    auto it = writeBuffers.find(fd);
    if (it == writeBuffers.end()) {
        logError(getClientInfo(fd) + " No data to write");
        return;
    }
    
    ssize_t n = write(fd, it->second.c_str(), it->second.length());
    if (n < 0) {
        logError(getClientInfo(fd) + " Write error: " + std::string(strerror(errno)));
        closeConnection(fd);
        return;
    }
    
    logDebug(getClientInfo(fd) + " Sent " + std::to_string(n) + " bytes");
    
    // ... 处理写完成后的逻辑 ...
}

// 在closeConnection方法中添加日志
void EpollServer::closeConnection(int fd) {
    logDebug(getClientInfo(fd) + " Closing connection");
    
    // 清理数据库连接
    SqliteConnectHandler::removeHandler(fd);
    epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    writeBuffers.erase(fd);
    
    logDebug(getClientInfo(fd) + " Connection closed");
} 