#pragma once
#include "string"
#include "set"
#include "boost/uuid/uuid.hpp"

struct User {
    boost::uuids::uuid id;
    std::string email;
    std::string nick;
};
