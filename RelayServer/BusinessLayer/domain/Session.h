#pragma once

#include <boost/uuid/uuid.hpp>

struct Session {
    boost::uuids::uuid userId;
    std::string token;
};

