#pragma once
#include "sqlite3_handler.h"
#include <json/json.h>
#include <map>
#include <memory>

class SqliteConnectHandler {
public:
    SqliteConnectHandler();
    std::string handle(const Json::Value& request, int clientFd);
    static Sqlite3Handler* getHandler(int clientFd);
    static void removeHandler(int clientFd);

private:
    static std::map<int, std::unique_ptr<Sqlite3Handler>> dbHandlers_;
}; 