#include "ChatService.h"

#include <utility>

namespace CPCDServer {

    ChatService::ChatService(std::shared_ptr<IChatRepository> chatRepository, std::shared_ptr<IUserService> userService)
            : chatRepository_(std::move(chatRepository)), userService_(std::move(userService)) {}

    std::expected<boost::uuids::uuid, CreateChatError>
    ChatService::CreateChat(const std::string &title, const std::vector<boost::uuids::uuid> &userIds,
                            const boost::uuids::uuid &creatorId) {
        for (const auto i: userIds) {
            if (i == creatorId) {
                continue;
            }

            if (!userService_->AreFriends(i, creatorId)) {
                return std::unexpected(CreateChatError::UsersAreNotFriendsOfCreator);
            }
        }

        try {
            Chat chat;
            chat.title = title;
            chat.users = userIds;
            chat.chat_type = ChatType::GROUP;

            return chatRepository_->CreateChat(chat);
        } catch (...) {
            return std::unexpected(CreateChatError::DatabaseFailure);
        }
    }

    void ChatService::UpdateChat(const Chat &chat) {
        chatRepository_->UpdateChat(chat);
    }

    void ChatService::DeleteChat(const boost::uuids::uuid &id) {
        chatRepository_->DeleteChat(id);
    }

    std::optional<Chat> ChatService::GetById(const boost::uuids::uuid &id) const {
        return chatRepository_->GetById(id);
    }

    std::vector<Chat> ChatService::GetByUser(const boost::uuids::uuid &id) const {
        return chatRepository_->GetByUser(id);
    }

} // CPCDServer