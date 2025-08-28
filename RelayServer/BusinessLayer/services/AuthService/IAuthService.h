#pragma once

#include "../../domain/Session.h"

#include <string>

namespace CPCDServer {

    class IAuthService {
    public:
        virtual ~IAuthService() = default;

        virtual boost::uuids::uuid
        registerUser(const std::string &email, const std::string &nickname, const std::string &password) = 0;

        virtual Session login(const std::string &email, const std::string &password) = 0;

        virtual bool validateToken(const std::string &token) = 0;

        virtual void logout(const std::string &token) = 0;
    };

} // CPCDServer