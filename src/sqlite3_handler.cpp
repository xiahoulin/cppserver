#include "sqlite3_handler.h"
#include <iostream>

Sqlite3Handler::Sqlite3Handler(const std::string& path) 
    : db(nullptr)
    , dbPath(path)
    , lastError() 
{
}

Sqlite3Handler::~Sqlite3Handler() {
    close();
}

bool Sqlite3Handler::open() {
    if (db) {
        return true;  // 已经打开
    }
    
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        lastError = sqlite3_errmsg(db);
        std::cerr << "Cannot open database: " << lastError << std::endl;
        sqlite3_close(db);
        db = nullptr;
        return false;
    }
    
    std::cout << "Successfully opened database: " << dbPath << std::endl;
    return true;
}

void Sqlite3Handler::close() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

bool Sqlite3Handler::beginTransaction() {
    return executeSql("BEGIN TRANSACTION;");
}

bool Sqlite3Handler::commitTransaction() {
    return executeSql("COMMIT TRANSACTION;");
}

bool Sqlite3Handler::rollback() {
    return executeSql("ROLLBACK TRANSACTION;");
}

bool Sqlite3Handler::executeSql(const std::string& sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        lastError = errMsg ? errMsg : "Unknown error";
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

TableData Sqlite3Handler::executeQuery(const std::string& sql) {
    TableData result;
    char* errMsg = nullptr;
    
    if (sqlite3_exec(db, sql.c_str(), callback, &result, &errMsg) != SQLITE_OK) {
        result.setStatus(-1);
        result.setMsg(errMsg ? errMsg : "Query failed");
        sqlite3_free(errMsg);
    } else {
        result.setStatus(0);
        result.setMsg("Query successful");
    }
    
    return result;
}

bool Sqlite3Handler::executeUpdate(const std::string& sql) {
    return executeSql(sql);
}

int Sqlite3Handler::callback(void* data, int argc, char** argv, char** azColName) {
    TableData* result = static_cast<TableData*>(data);
    std::map<std::string, std::string> row;
    
    // 第一行数据时，添加列信息
    if (result->getRowCount() == 0) {
        for (int i = 0; i < argc; i++) {
            // 获取列的实际类型
            result->addColumnType(azColName[i], "TEXT"); // 简化处理，实际应该从schema获取
        }
    }
    
    // 添加行数据
    for (int i = 0; i < argc; i++) {
        row[azColName[i]] = argv[i] ? argv[i] : "NULL";
    }
    
    result->addRowValue(row);
    return 0;
}

int Sqlite3Handler::getAffectedRows() const {
    return sqlite3_changes(db);
} 