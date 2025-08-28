#pragma once

#include "../domain/Chat.h"
#include <boost/uuid/uuid.hpp>

class IChatRepository {
public:
    virtual ~IChatRepository()=default;

    virtual const boost::uuids::uuid& CreateChat(const Chat& chat) = 0;
    virtual std::optional<Chat> GetById(const boost::uuids::uuid& id) = 0;
    virtual std::vector<Chat> GetByUser(const boost::uuids::uuid& id) = 0;

    virtual void UpdateChat(const Chat& message) = 0;
    virtual void DeleteChat(const boost::uuids::uuid& id) = 0;
};