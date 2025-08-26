#pragma once

#include "iostream"
#include <boost/uuid/uuid.hpp>
#include "string"
#include "set"

struct Message {
    boost::uuids::uuid id;
    boost::uuids::uuid UUID;
    boost::uuids::uuid senderId;
    std::string body;
    std::chrono::system_clock::time_point createdAt;
    std::set<boost::uuids::uuid> deliveredTo;
    std::set<boost::uuids::uuid> readBy;
};
