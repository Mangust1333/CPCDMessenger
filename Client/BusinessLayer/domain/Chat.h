#pragma once

#include <string>
#include <set>
#include <vector>
#include <boost/uuid/uuid.hpp>
#include "ChatType.h"
#include "Message.h"

struct Chat {
    boost::uuids::uuid id;
    std::string title;
    std::vector<boost::uuids::uuid> users;
    std::vector<Message> messages;
    ChatType chat_type;
};