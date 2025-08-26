#pragma once

#include <boost/uuid/uuid.hpp>

struct FriendRelation {
    const boost::uuids::uuid& id;
    const boost::uuids::uuid& user1;
    const boost::uuids::uuid& user2;
};