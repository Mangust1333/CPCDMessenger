#pragma once


#include "../domain/Message.h"
#include <boost/uuid/uuid.hpp>

class IMessageRepository {
public:
    virtual ~IMessageRepository()=default;


    virtual const boost::uuids::uuid& CreateMessage(const Message& message) = 0;
    virtual std::optional<Message> GetById(const boost::uuids::uuid& id) = 0;
    virtual std::vector<Message> GetByChatId(const boost::uuids::uuid& id) = 0;
    virtual std::vector<Message> GetByNick(const std::string& nick) = 0;
    virtual void UpdateMessage(const Message& message) = 0;
    virtual void DeleteMessage(const boost::uuids::uuid& id) = 0;

};
