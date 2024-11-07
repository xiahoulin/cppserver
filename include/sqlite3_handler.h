#pragma once
#include <sqlite3.h>
#include <string>
#include "table_data.h"

/**
 * @brief SQLite3数据库操作封装类
 * 提供数据库连接、查询等基本操作
 */
class Sqlite3Handler {
public:
    /**
     * @brief 构造函数
     * @param dbPath 数据库文件路径
     */
    explicit Sqlite3Handler(const std::string& dbPath);
    ~Sqlite3Handler();
    
    /**
     * @brief 禁用拷贝
     */
    Sqlite3Handler(const Sqlite3Handler&) = delete;
    Sqlite3Handler& operator=(const Sqlite3Handler&) = delete;
    
    /**
     * @brief 打开数据库连接
     * @return 是否成功打开
     */
    bool open();

    /**
     * @brief 关闭数据库连接
     */
    void close();

    /**
     * @brief 执行查询语句
     * @param sql SQL查询语句
     * @return TableData对象，包含查询结果
     */
    TableData executeQuery(const std::string& sql);

    /**
     * @brief 执行更新语句（INSERT、UPDATE、DELETE等）
     * @param sql SQL更新语句
     * @return 是否执行成功
     */
    bool executeUpdate(const std::string& sql);

    /**
     * @brief 开始事务
     * @return 是否成功开始事务
     */
    bool beginTransaction();

    /**
     * @brief 提交事务
     * @return 是否成功提交事务
     */
    bool commitTransaction();

    /**
     * @brief 回滚事务
     * @return 是否成功回滚事务
     */
    bool rollback();

    /**
     * @brief 获取最后的错误信息
     * @return 最后的错误信息
     */
    std::string getLastError() const { return lastError; }

    /**
     * @brief 获取影响行数
     * @return 影响行数
     */
    int getAffectedRows() const;

private:
    sqlite3* db;                    // SQLite3数据库连接句柄
    const std::string dbPath;       // 数据库文件路径
    std::string lastError;          // 最后的错误信息
    
    /**
     * @brief SQLite3查询回调函数
     * @param data 用户数据指针
     * @param argc 列数
     * @param argv 列值数组
     * @param azColName 列名数组
     * @return 0表示继续执行，非0表示中断执行
     */
    static int callback(void* data, int argc, char** argv, char** azColName);

    /**
     * @brief 执行SQL语句的通用方法
     * @param sql SQL语句
     * @return 是否执行成功
     */
    bool executeSql(const std::string& sql);
}; 