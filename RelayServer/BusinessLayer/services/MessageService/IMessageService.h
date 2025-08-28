#pragma once

#include "../../domain/Message.h"
#include <boost/uuid/uuid.hpp>

namespace CPCDServer {

    class IMessageService {
    public:
        virtual ~IMessageService()=default;

        virtual const boost::uuids::uuid &
        CreateMessage(const boost::uuids::uuid &chatId, const boost::uuids::uuid &senderId,
                      const std::string &body) = 0;
        virtual std::optional<Message> GetById(const boost::uuids::uuid& id) = 0;
        virtual std::vector<Message> GetByChatId(const boost::uuids::uuid& id) = 0;
        virtual std::vector<Message> GetByNick(const std::string& nick) = 0;
        virtual void UpdateMessage(const Message& message) = 0;
        virtual void DeleteMessage(const boost::uuids::uuid& id) = 0;

    };

} // CPCDServer

