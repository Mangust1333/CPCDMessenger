#include "MessageService.h"

#include <utility>

namespace CPCDServer {

    MessageService::MessageService(std::shared_ptr<IMessageRepository> messageRepository)
    : messageRepository_(std::move(messageRepository)) {}

    const boost::uuids::uuid & MessageService::CreateMessage(const boost::uuids::uuid &chatId,
                                                             const boost::uuids::uuid &senderId,
                                                             const std::string &body) {
        Message message;
        message.chatId = chatId;
        message.senderId = senderId;
        message.body = body;

        return messageRepository_->CreateMessage(message);
    }

    std::optional<Message> MessageService::GetById(const boost::uuids::uuid &id) {
        return messageRepository_->GetById(id);
    }

    std::vector<Message> MessageService::GetByChatId(const boost::uuids::uuid &id) {
        return messageRepository_->GetByChatId(id);
    }

    std::vector<Message> MessageService::GetByNick(const std::string &nick) {
        return messageRepository_->GetByNick(nick);
    }

    void MessageService::UpdateMessage(const Message &message) {
        messageRepository_->UpdateMessage(message);
    }

    void MessageService::DeleteMessage(const boost::uuids::uuid &id) {
        messageRepository_->DeleteMessage(id);
    }


} // CPCDServer