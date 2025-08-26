#pragma once
#include <string>
#include "nlohmann/json.hpp"

class ISession {
public:
    virtual ~ISession() = default;
    virtual void deliver_json(const nlohmann::json& j) = 0;
    virtual std::string username() const = 0;
    virtual void close() = 0;
};
