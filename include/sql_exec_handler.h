#pragma once
#include "./sqlite3_handler.h"
#include <string>
#include <vector>
#include <json/json.h>

class SqlExecHandler {
public:
    SqlExecHandler();
    
    std::string handle(const Json::Value& parsedRequest, int clientFd);

private:
    std::vector<std::string> splitSqlStatements(const std::string& sqlStr);
    bool isQueryStatement(const std::string& sql);
    bool isDeleteStatement(const std::string& sql);
    bool isInsertStatement(const std::string& sql);
    bool needTransaction(const std::vector<std::string>& statements);
}; 