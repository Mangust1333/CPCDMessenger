#pragma once

#include <expected>
#include "boost/uuid/uuid.hpp"
#include "UserServerErrors/CreateUserError.h"
#include "UserServerErrors/UpdateUserError.h"
#include "UserServerErrors/AddFriendError.h"

#include "../../domain/User.h"

class IUserService {
public:
    virtual ~IUserService() = default;
    virtual std::expected<boost::uuids::uuid, CreateUserError> CreateUser(const User& user) = 0;
    virtual std::expected<void, UpdateUserError> UpdateUser(const User& user) = 0;

    virtual std::optional<User> GetById(const boost::uuids::uuid& id) const = 0;
    virtual std::optional<User> GetByEmail(const std::string& email) const = 0;
    virtual std::optional<User> GetByNick(const std::string& nick) const = 0;

    virtual std::expected<boost::uuids::uuid, AddFriendError> AddFriend(const boost::uuids::uuid& userId1, const boost::uuids::uuid& userId2) = 0;
    virtual void RemoveFriend(const boost::uuids::uuid& relationId) = 0;
    virtual bool AreFriends(const boost::uuids::uuid& userId1, const boost::uuids::uuid& userId2) const = 0;
    virtual std::vector<User> ListFriends(const boost::uuids::uuid& userId) const = 0;
    virtual size_t CountFriends(const boost::uuids::uuid& userId) const = 0;

    virtual std::vector<User> ListUsers(size_t limit, size_t offset) const = 0;
};
