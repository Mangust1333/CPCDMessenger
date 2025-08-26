#pragma once
#include <optional>
#include <string>
#include <vector>
#include "../domain/User.h"

struct IUserRepository {
    virtual ~IUserRepository() = default;
    virtual std::optional<User> GetById(const std::string& id) = 0;
    virtual void Save(const User& user) = 0;
};
