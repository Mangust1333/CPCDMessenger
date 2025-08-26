#pragma once

#include "optional"
#include "string"
#include "../../domain/User.h"
#include "../../repository/IUserRepository.h"
#include "../../repository/IFriendRepository.h"
#include "boost/uuid/uuid.hpp"
#include "IUserService.h"

namespace CPCDServer {

    class UserService : IUserService {
    public:
        explicit UserService(
                std::shared_ptr<IUserRepository> userRepository,
                std::shared_ptr<IFriendRepository> friendRepository) noexcept;

        std::expected<boost::uuids::uuid, CreateUserError> CreateUser(const User& user) override;
        std::expected<void, UpdateUserError> UpdateUser(const User& user) override;

        std::optional<User> GetById(const boost::uuids::uuid& id) const override;
        std::optional<User> GetByEmail(const std::string& email) const override;
        std::optional<User> GetByNick(const std::string& nick) const override;

        std::expected<boost::uuids::uuid, AddFriendError> AddFriend(const boost::uuids::uuid& userId1, const boost::uuids::uuid& userId2) override;
        void RemoveFriend(const boost::uuids::uuid& relationId) override;
        bool AreFriends(const boost::uuids::uuid& userId1, const boost::uuids::uuid& userId2) const override;
        std::vector<User> ListFriends(const boost::uuids::uuid& userId) const override;
        size_t CountFriends(const boost::uuids::uuid& userId) const override;

        std::vector<User> ListUsers(size_t limit, size_t offset) const override;

    private:
        std::shared_ptr<IFriendRepository> friendRepository_;
        std::shared_ptr<IUserRepository> userRepository_;
    };

} // CPCDServer
