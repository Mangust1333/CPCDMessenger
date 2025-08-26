#include "UserService.h"

#include <utility>

namespace CPCDServer {
    UserService::UserService(std::shared_ptr<IUserRepository> userRepository,
                             std::shared_ptr<IFriendRepository> friendRepository) noexcept
                             : userRepository_(std::move(userRepository)),
                             friendRepository_(std::move(friendRepository)) {}

    std::expected<boost::uuids::uuid, CreateUserError> UserService::CreateUser(const User &user) {
        std::optional<User> userByEmail = userRepository_->GetByEmail(user.email);
        if(userByEmail.has_value()) {
            return std::unexpected(CreateUserError::EmailAlreadyExists);
        }

        try {
            auto id = userRepository_->CreateUser(user);
            return id;
        } catch (...) {
            return std::unexpected(CreateUserError::DatabaseFailure);
        }
    }

    std::expected<void, UpdateUserError> UserService::UpdateUser(const User &user) {
        std::optional<User> userByEmail = userRepository_->GetByEmail(user.email);
        if(userByEmail.has_value() && userByEmail->id != user.id) {
            return std::unexpected(UpdateUserError::EmailAlreadyExists);
        }

        std::optional<User> userById = userRepository_->GetById(user.id);
        if(!userById.has_value()) {
            return std::unexpected(UpdateUserError::UserDoNotExists);
        }

        try {
            userRepository_->UpdateUser(user);
        } catch (...) {
            return std::unexpected(UpdateUserError::DatabaseFailure);
        }

        return {};
    }

    std::optional<User> UserService::GetById(const boost::uuids::uuid &id) const {
        return userRepository_->GetById(id);
    }

    std::optional<User> UserService::GetByEmail(const std::string &email) const {
        return userRepository_->GetByEmail(email);
    }

    std::optional<User> UserService::GetByNick(const std::string &nick) const {
        return userRepository_->GetByNick(nick);
    }

    std::expected<boost::uuids::uuid, AddFriendError> UserService::AddFriend(const boost::uuids::uuid &userId1, const boost::uuids::uuid &userId2) {
        if(!userRepository_->GetById(userId1).has_value() || !userRepository_->GetById(userId2).has_value()) {
            return std::unexpected(AddFriendError::UserDoNotExists);
        }

        if(!friendRepository_->AreFriends(userId1, userId2)) {
            return std::unexpected(AddFriendError::UsersAlreadyFriends);
        }

        try {
            return friendRepository_->AddFriend(userId1, userId2);
        } catch (...) {
            return std::unexpected(AddFriendError::DatabaseFailure);
        }
    }

    std::vector<User> UserService::ListUsers(size_t limit, size_t offset) const {
        return userRepository_->ListUsers(limit, offset);
    }

    void UserService::RemoveFriend(const boost::uuids::uuid &relationId) {
        return friendRepository_->RemoveFriend(relationId);
    }

    bool UserService::AreFriends(const boost::uuids::uuid &userId1, const boost::uuids::uuid &userId2) const {
        return friendRepository_->AreFriends(userId1, userId2);
    }

    std::vector<User> UserService::ListFriends(const boost::uuids::uuid &userId) const {
        return friendRepository_->ListFriends(userId);
    }

    size_t UserService::CountFriends(const boost::uuids::uuid &userId) const {
        return friendRepository_->ListFriends(userId).size();
    }

} // CPCDServer