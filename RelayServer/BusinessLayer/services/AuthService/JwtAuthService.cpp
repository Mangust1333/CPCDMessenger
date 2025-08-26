#include "JwtAuthService.h"
#include <stdexcept>
#include <sstream>

#include <iostream>
#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/nlohmann-json/defaults.h>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/string_generator.hpp>
#include <utility>

using namespace std::chrono;

namespace CPCDServer {

    JwtAuthService::JwtAuthService(std::shared_ptr<IUserService> userService,
                                   std::shared_ptr<ICredentialRepository> credentialRepository,
                                   std::string authSecret_, uint64_t ttlSeconds)
            : userService(std::move(userService)),
            credentialRepository(std::move(credentialRepository)),
            authSecret(std::move(authSecret_)),
            tokenTtlSeconds(ttlSeconds)
            {
        if (authSecret.empty()) {
            throw std::runtime_error("AuthService: empty secret is unsafe");
        }
    }

    static system_clock::time_point now_tp() {
        return system_clock::now();
    }

    boost::uuids::uuid JwtAuthService::registerUser(const std::string &email,
                                                    const std::string &nickname,
                                                    const std::string &password) {
        std::lock_guard<std::mutex> lk(mu);

        User user;
        user.email = email;
        user.nick = nickname;
        auto userIdResult = userService->CreateUser(user);

        if (!userIdResult) {
            if (userIdResult.error() == CreateUserError::EmailAlreadyExists) {
                std::cerr << "Пользователь уже существует\n";
            } else if(userIdResult.error() == CreateUserError::EmailAlreadyExists) {
                std::cerr << "Ошибка базы данных\n";
            } else {
                std::cerr << "Неизвестная ошибка\n";
            }
        } else {
            std::cout << "Создан пользователь " << userIdResult.value() << "\n";
        }

        Credential credential;
        credential.userId = userIdResult.value();
        credential.passwordHash = passwordHasher->Hash(password, "");
        credentialRepository->CreateCredential(credential);

        return userIdResult.value();
    }

    Session JwtAuthService::login(const std::string &email,
                                  const std::string &password) {
        std::lock_guard<std::mutex> lk(mu);
        auto userOpt = userService->GetByEmail(email);
        if (!userOpt.has_value()) {
            throw std::runtime_error("User not found");
        }
        auto user = *userOpt;

        auto credOpt = credentialRepository->GetByUserId(user.id);
        if (!credOpt.has_value()) {
            throw std::runtime_error("Credential not found for user");
        }
        const auto &storedHash = credOpt->passwordHash;

        if (!passwordHasher->Verify(storedHash, password, "")) {
            throw std::runtime_error("Invalid password");
        }

        auto token = jwt::create()
                .set_issuer("relay-server")
                .set_type("JWS")
                .set_issued_at(system_clock::now())
                .set_expires_at(system_clock::now() + seconds(static_cast<int>(tokenTtlSeconds)))
                .set_subject(boost::uuids::to_string(user.id))
                .set_payload_claim("email", jwt::claim(user.email))
                .sign(jwt::algorithm::hs256{authSecret});

        Session s;
        s.userId = user.id;
        s.token = token;

        return s;
    }

    bool JwtAuthService::validateToken(const std::string &token) {
        try {
            {
                std::lock_guard<std::mutex> lk(mu);
                this->cleanupRevoked();
                auto rit = revokedTokens.find(token);
                if (rit != revokedTokens.end()) {
                    return false;
                }
            }

            auto decoded = jwt::decode(token);
            auto verifier = jwt::verify()
                    .allow_algorithm(jwt::algorithm::hs256{authSecret})
                    .with_issuer("relay-server");
            verifier.verify(decoded);

            if (decoded.has_expires_at()) {
                auto exp_tp = decoded.get_expires_at();
                if (exp_tp < system_clock::now()) {
                    return false;
                }
            } else {
                return false;
            }

            if (decoded.has_subject()) {
                const std::string subj = decoded.get_subject();
                try {
                    boost::uuids::string_generator gen;
                    boost::uuids::uuid uid = gen(subj);
                    auto u = userService->GetById(uid);
                    if (!u.has_value()) {
                        return false;
                    }
                } catch (const std::exception& ex) {
                    return false;
                }
            } else {
                return false;
            }

            return true;

        } catch (const std::exception &ex) {
            (void)ex;
            return false;
        }
    }


    void JwtAuthService::logout(const std::string &token) {
        try {
            auto decoded = jwt::decode(token);
            system_clock::time_point exp_tp = system_clock::now();
            if (decoded.has_expires_at()) {
                exp_tp = decoded.get_expires_at();
            } else {
                exp_tp = system_clock::now() + seconds(static_cast<int>(tokenTtlSeconds));
            }

            std::lock_guard<std::mutex> lk(mu);
            revokedTokens[token] = exp_tp;
            cleanupRevoked();
        } catch (...) {
        }
    }

    void JwtAuthService::cleanupRevoked() {
        auto now = now_tp();
        std::vector<std::string> toErase;
        for (auto &p: revokedTokens) {
            if (p.second <= now) toErase.push_back(p.first);
        }
        for (auto &t: toErase) revokedTokens.erase(t);
    }


}