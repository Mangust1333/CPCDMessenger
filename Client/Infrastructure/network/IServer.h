#pragma once
#include <memory>
#include <string>
#include "nlohmann/json.hpp"

class ISession;

class IServer {
public:
    virtual ~IServer() = default;
    virtual void register_username(const std::string& user, std::shared_ptr<ISession> session) = 0;
    virtual void unregister_username(const std::string& user) = 0;
    virtual void route_message(const std::string& to, const nlohmann::json& message) = 0;
};
