#include "sqlite_connect_handler.h"
#include <memory>

std::map<int, std::unique_ptr<Sqlite3Handler>> SqliteConnectHandler::dbHandlers_;

SqliteConnectHandler::SqliteConnectHandler() {}

std::string SqliteConnectHandler::handle(const Json::Value& request, int clientFd) {
    Json::Value response;
    response["status"] = -1;

    try {
        if (!request.isMember("msg") || !request["msg"].isMember("dbpath")) {
            response["msg"] = "Missing dbpath parameter";
            return Json::FastWriter().write(response);
        }

        std::string dbPath = request["msg"]["dbpath"].asString();
        std::unique_ptr<Sqlite3Handler> handler(new Sqlite3Handler(dbPath));
        
        if (!handler->open()) {
            response["msg"] = "Failed to open database: " + handler->getLastError();
            return Json::FastWriter().write(response);
        }

        auto it = dbHandlers_.find(clientFd);
        if (it != dbHandlers_.end()) {
            dbHandlers_.erase(it);
        }

        dbHandlers_[clientFd] = std::move(handler);
        
        response["status"] = 0;
        response["msg"] = "Database connection established successfully";
        
    } catch (const std::exception& e) {
        response["msg"] = std::string("Exception occurred: ") + e.what();
    }

    return Json::FastWriter().write(response);
}

Sqlite3Handler* SqliteConnectHandler::getHandler(int clientFd) {
    auto it = dbHandlers_.find(clientFd);
    return it != dbHandlers_.end() ? it->second.get() : nullptr;
}

void SqliteConnectHandler::removeHandler(int clientFd) {
    dbHandlers_.erase(clientFd);
} 