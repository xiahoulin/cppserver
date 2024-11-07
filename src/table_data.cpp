#include "table_data.h"

TableData::TableData() : status(0) {}

TableData::~TableData() {}

void TableData::setStatus(int status) {
    this->status = status;
}

void TableData::setMsg(const std::string& msg) {
    this->msg = msg;
}

void TableData::addColumnType(const std::string& column, const std::string& type) {
    colAndType[column] = type;
}

void TableData::addRowValue(const std::map<std::string, std::string>& row) {
    rowAndValue.push_back(row);
}

std::string TableData::toJson() const {
    Json::Value root;
    root["status"] = status;
    root["msg"] = msg;
    
    Json::Value columns;
    for (const auto& col : colAndType) {
        columns[col.first] = col.second;
    }
    root["columns"] = columns;
    
    Json::Value rows(Json::arrayValue);
    for (const auto& row : rowAndValue) {
        Json::Value rowObj;
        for (const auto& field : row) {
            rowObj[field.first] = field.second;
        }
        rows.append(rowObj);
    }
    root["rows"] = rows;
    
    Json::FastWriter writer;
    return writer.write(root);
} 