#include "epoll_server.h"
#include "sql_exec_handler.h"
#include "sqlite_connect_handler.h"
#include <memory>
#include <iostream>

int main() {
    try {
        EpollServer server(8083);
        
        // 创建数据库连接处理器
        auto sqliteConnectHandler = std::make_shared<SqliteConnectHandler>();
        server.registerHandler("100000", [sqliteConnectHandler](const Json::Value& request, int clientFd) {
            return sqliteConnectHandler->handle(request, clientFd);
        });
        
        // SQL执行处理器
        auto sqlExecHandler = std::make_shared<SqlExecHandler>();
        server.registerHandler("100001", [sqlExecHandler](const Json::Value& request, int clientFd) {
            return sqlExecHandler->handle(request, clientFd);
        });
        
        std::cout << "Server starting on port 8083..." << std::endl;
        server.start();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 