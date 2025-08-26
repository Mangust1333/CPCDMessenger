#pragma once

#include "../domain/FriendRelation.h"
#include "../domain/User.h"

class IFriendRepository {
public:
    virtual ~IFriendRepository() = default;

    virtual const boost::uuids::uuid& AddFriend(const boost::uuids::uuid& userId1, const boost::uuids::uuid& userId2) = 0;
    virtual void RemoveFriend(const boost::uuids::uuid& relationId) = 0;
    virtual bool AreFriends(const boost::uuids::uuid& userId1, const boost::uuids::uuid& userId2) = 0;
    virtual std::vector<User> ListFriends(const boost::uuids::uuid& userId) = 0;
};

