#pragma once
#include <optional>
#include <string>
#include <vector>
#include <memory>
#include "boost/uuid/uuid.hpp"

#include "../domain/User.h"

class IUserRepository {
public:
    virtual ~IUserRepository() = default;

    virtual const boost::uuids::uuid& CreateUser(const User& user) = 0;
    virtual std::optional<User> GetById(const boost::uuids::uuid& id) = 0;
    virtual std::optional<User> GetByEmail(const std::string& email) = 0;
    virtual std::optional<User> GetByNick(const std::string& nick) = 0;
    virtual void UpdateUser(const User& user) = 0;
    virtual void DeleteUser(const boost::uuids::uuid& id) = 0;
    virtual std::vector<User> ListUsers(size_t limit, size_t offset) = 0;
};
