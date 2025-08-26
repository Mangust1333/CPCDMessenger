#pragma once

#include <unordered_map>
#include <mutex>
#include "IAuthService.h"
#include "../../domain/User.h"
#include "../UserService/UserService.h"
#include "boost/uuid/uuid.hpp"
#include "../../repository/ICredentialRepository.h"
#include "../../contracts/IPasswordHasher.h"

namespace CPCDServer {

    class JwtAuthService : public IAuthService {
        std::shared_ptr<IUserService> userService;
        std::shared_ptr<ICredentialRepository> credentialRepository;
        std::shared_ptr<IPasswordHasher> passwordHasher;

        std::unordered_map<std::string, std::chrono::system_clock::time_point> revokedTokens;
        std::string authSecret;
        uint64_t tokenTtlSeconds;

        std::mutex mu;
    public:
        explicit JwtAuthService(std::shared_ptr<IUserService> userService,
                                std::shared_ptr<ICredentialRepository> credentialRepository,
                                std::string authSecret_,
                                uint64_t ttlSeconds);

        boost::uuids::uuid registerUser(const std::string& email,
                                        const std::string& nickname,
                                        const std::string& password) override;
        Session login(const std::string& email, const std::string& password) override;
        bool validateToken(const std::string& token) override;
        void logout(const std::string& token) override;

    private:
        void cleanupRevoked();
    };

} // CPCDServer
