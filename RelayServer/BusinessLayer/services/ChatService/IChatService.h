#pragma once

#include <expected>
#include "IChatService.h"
#include "../../repository/IChatRepository.h"
#include "ChatServiceErrors/CreateChatError.h"

namespace CPCDServer {

    class IChatService {
    public:
        virtual ~IChatService() = default;

        virtual std::expected<boost::uuids::uuid, CreateChatError>
        CreateChat(const std::string &title, const std::vector<boost::uuids::uuid> &userIds,
                   const boost::uuids::uuid &creatorId) = 0;

        virtual void UpdateChat(const Chat &chat) = 0;

        virtual void DeleteChat(const boost::uuids::uuid &id) = 0;

        virtual std::optional<Chat> GetById(const boost::uuids::uuid &id) const = 0;

        virtual std::vector<Chat> GetByUser(const boost::uuids::uuid &id) const = 0;
    };

} // CPCDServer