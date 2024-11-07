#pragma once
#include <sys/epoll.h>
#include <string>
#include <functional>
#include <map>
#include <memory>
#include "sqlite3_handler.h"
#include <sstream>
#include <ctime>
#include "json/json.h"

/**
 * @brief Epoll服务器类
 */
class EpollServer {
public:
    /**
     * @brief 构造函数
     * @param port 服务器监听端口
     */
    explicit EpollServer(int port);
    ~EpollServer();
    
    void start();
    void registerHandler(const std::string& funcId, 
                        std::function<std::string(const Json::Value&, int)> handler);
    
private:
    int serverFd;
    int epollFd;
    int listenFd;
    std::map<std::string, std::function<std::string(const Json::Value&, int)>> handlers;
    std::map<int, std::string> writeBuffers;
    
    void handleAccept();
    void handleRead(int fd);
    void handleWrite(int fd);
    void closeConnection(int fd);
    
    void initServer();
    void handleConnection(int clientFd);
    std::string processRequest(const std::string& request, int clientFd);
    
    void logDebug(const std::string& message) const;
    void logError(const std::string& message) const;
    std::string getCurrentTimestamp() const;
    std::string getClientInfo(int clientFd) const;
}; 