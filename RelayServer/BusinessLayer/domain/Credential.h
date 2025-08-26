#pragma once

#include <string>
#include <boost/uuid/uuid.hpp>

struct Credential {
    boost::uuids::uuid userId;
    std::string passwordHash;
};
