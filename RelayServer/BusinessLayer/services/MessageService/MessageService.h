#pragma once

#include "IMessageService.h"
#include "../../repository/IMessageRepository.h"

namespace CPCDServer {

    class MessageService : IMessageService {
        std::shared_ptr<IMessageRepository> messageRepository_;
    public:
        explicit MessageService(std::shared_ptr<IMessageRepository> messageRepository);

        const boost::uuids::uuid &CreateMessage(const boost::uuids::uuid &chatId, const boost::uuids::uuid &senderId,
                                                const std::string &body) override;
        void UpdateMessage(const Message& message) override;
        void DeleteMessage(const boost::uuids::uuid& id) override;

        std::optional<Message> GetById(const boost::uuids::uuid& id) override;
        std::vector<Message> GetByChatId(const boost::uuids::uuid& id) override;
        std::vector<Message> GetByNick(const std::string& nick) override;
    };

} // CPCDServer

