#pragma once

#include <expected>
#include "IChatService.h"
#include "../../repository/IChatRepository.h"
#include "../UserService/IUserService.h"


namespace CPCDServer {

    class ChatService : IChatService {
        std::shared_ptr<IChatRepository> chatRepository_;
        std::shared_ptr<IUserService> userService_;
    public:

        explicit ChatService(std::shared_ptr<IChatRepository> chatRepository,
                             std::shared_ptr<IUserService> userService);

        std::expected<boost::uuids::uuid, CreateChatError>
        CreateChat(const std::string &title, const std::vector<boost::uuids::uuid> &userIds,
                   const boost::uuids::uuid &creatorId) override;

        void UpdateChat(const Chat &chat) override;

        void DeleteChat(const boost::uuids::uuid &id) override;

        std::optional<Chat> GetById(const boost::uuids::uuid &id) const override;

        std::vector<Chat> GetByUser(const boost::uuids::uuid &id) const override;
    };

} // CPCDServer