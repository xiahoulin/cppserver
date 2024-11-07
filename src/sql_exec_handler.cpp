#include "sql_exec_handler.h"
#include "sqlite_connect_handler.h"
#include <json/json.h>
#include <sstream>
#include <algorithm>
#include <cctype>

SqlExecHandler::SqlExecHandler() {}

std::vector<std::string> SqlExecHandler::splitSqlStatements(const std::string& sqlStr) {
    std::vector<std::string> statements;
    std::istringstream ss(sqlStr);
    std::string statement;
    std::string temp;
    
    while (std::getline(ss, temp, ';')) {
        temp.erase(0, temp.find_first_not_of(" \n\r\t"));
        temp.erase(temp.find_last_not_of(" \n\r\t") + 1);
        
        if (!temp.empty()) {
            statement += temp + ";";
            statements.push_back(statement);
            statement.clear();
        }
    }
    
    return statements;
}

bool SqlExecHandler::isQueryStatement(const std::string& sql) {
    std::string lowerSql = sql;
    std::transform(lowerSql.begin(), lowerSql.end(), lowerSql.begin(), ::tolower);
    return lowerSql.find("select") != std::string::npos;
}

bool SqlExecHandler::isDeleteStatement(const std::string& sql) {
    std::string lowerSql = sql;
    std::transform(lowerSql.begin(), lowerSql.end(), lowerSql.begin(), ::tolower);
    return lowerSql.find("delete") != std::string::npos;
}

bool SqlExecHandler::isInsertStatement(const std::string& sql) {
    std::string lowerSql = sql;
    std::transform(lowerSql.begin(), lowerSql.end(), lowerSql.begin(), ::tolower);
    return lowerSql.find("insert") != std::string::npos;
}

bool SqlExecHandler::needTransaction(const std::vector<std::string>& statements) {
    if (statements.size() > 1) {
        return true;
    }
    
    const std::string& sql = statements[0];
    return !isQueryStatement(sql);
}

std::string SqlExecHandler::handle(const Json::Value& parsedRequest, int clientFd) {
    TableData result;
    
    try {
        Sqlite3Handler* dbHandler = SqliteConnectHandler::getHandler(clientFd);
        if (!dbHandler) {
            result.setStatus(-1);
            result.setMsg("Database connection not initialized");
            return result.toJson();
        }

        if (!parsedRequest.isMember("msg") || !parsedRequest["msg"].isMember("sqlstr")) {
            result.setStatus(-1);
            result.setMsg("Missing sqlstr in request");
            return result.toJson();
        }
        
        std::string sqlStr = parsedRequest["msg"]["sqlstr"].asString();
        if (sqlStr.empty()) {
            result.setStatus(-1);
            result.setMsg("Empty SQL statement");
            return result.toJson();
        }

        std::vector<std::string> sqlStatements = splitSqlStatements(sqlStr);
        if (sqlStatements.empty()) {
            result.setStatus(-1);
            result.setMsg("No valid SQL statements");
            return result.toJson();
        }

        bool useTransaction = needTransaction(sqlStatements);
        bool hasExplicitTransaction = false;
        
        for (const auto& sql : sqlStatements) {
            std::string lowerSql = sql;
            std::transform(lowerSql.begin(), lowerSql.end(), lowerSql.begin(), ::tolower);
            if (lowerSql.find("begin") != std::string::npos) {
                hasExplicitTransaction = true;
                break;
            }
        }

        if (useTransaction && !hasExplicitTransaction) {
            if (!dbHandler->beginTransaction()) {
                result.setStatus(-1);
                result.setMsg("Failed to begin transaction");
                return result.toJson();
            }
        }

        for (const auto& sql : sqlStatements) {
            if (isQueryStatement(sql)) {
                result = dbHandler->executeQuery(sql);
                if (result.getStatus() != 0) {
                    if (useTransaction && !hasExplicitTransaction) {
                        dbHandler->rollback();
                    }
                    return result.toJson();
                }
            } else {
                bool success = dbHandler->executeUpdate(sql);
                if (!success) {
                    result.setStatus(-1);
                    std::string operation;
                    if (isDeleteStatement(sql)) {
                        operation = "Delete";
                    } else if (isInsertStatement(sql)) {
                        operation = "Insert";
                    } else {
                        operation = "Update";
                    }
                    result.setMsg(operation + " operation failed: " + dbHandler->getLastError());
                    if (useTransaction && !hasExplicitTransaction) {
                        dbHandler->rollback();
                    }
                    return result.toJson();
                }
                result.setStatus(0);
                std::string operation;
                if (isDeleteStatement(sql)) {
                    operation = "Delete";
                    result.setAffectedRows(dbHandler->getAffectedRows());
                } else if (isInsertStatement(sql)) {
                    operation = "Insert";
                    result.setAffectedRows(dbHandler->getAffectedRows());
                } else {
                    operation = "Update";
                }
                result.setMsg(operation + " successful");
            }
        }

        if (useTransaction && !hasExplicitTransaction) {
            if (!dbHandler->commitTransaction()) {
                result.setStatus(-1);
                result.setMsg("Failed to commit transaction");
                dbHandler->rollback();
                return result.toJson();
            }
        }

        return result.toJson();
        
    } catch (const std::exception& e) {
        result.setStatus(-1);
        result.setMsg(std::string("Exception occurred: ") + e.what());
        return result.toJson();
    }
} 