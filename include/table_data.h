#pragma once
#include <string>
#include <vector>
#include <map>
#include <json/json.h>

/**
 * @brief 数据表结构类，用于存储数据库查询结果
 * 包含查询状态、消息、列信息和行数据
 */
class TableData {
public:
    TableData();
    ~TableData();
    
    /**
     * @brief 设置查询状态
     * @param status 状态码（0表示成功，非0表示失败）
     */
    void setStatus(int status);

    /**
     * @brief 设置查询结果消息
     * @param msg 结果消息
     */
    void setMsg(const std::string& msg);

    /**
     * @brief 添加列名和对应的数据类型
     * @param column 列名
     * @param type 数据类型
     */
    void addColumnType(const std::string& column, const std::string& type);

    /**
     * @brief 添加一行数据
     * @param row 键值对形式的行数据，key为列名，value为列值
     */
    void addRowValue(const std::map<std::string, std::string>& row);

    /**
     * @brief 将查询结果序列化为JSON字符串
     * @return JSON格式的字符串
     */
    std::string toJson() const;

    size_t getRowCount() const { return rowAndValue.size(); }
    int getStatus() const { return status; }

    /**
     * @brief 设置影响行数
     * @param rows 影响行数
     */
    void setAffectedRows(int rows) { affectedRows = rows; }

private:
    int status;                     // 查询状态
    std::string msg;                // 查询消息
    std::map<std::string, std::string> colAndType;      // 列名和类型的映射
    std::vector<std::map<std::string, std::string>> rowAndValue;  // 行数据数组
    int affectedRows;
}; 