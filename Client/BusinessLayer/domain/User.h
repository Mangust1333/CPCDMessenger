#pragma once

#include <boost/uuid/uuid.hpp>
#include <string>
#include <set>

struct User {
    boost::uuids::uuid id_;
    std::string nick;
};