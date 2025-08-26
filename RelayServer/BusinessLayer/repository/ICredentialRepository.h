#pragma once
#include <optional>
#include <string>
#include <boost/uuid/uuid.hpp>
#include "../domain/Credential.h"

class ICredentialRepository {
public:
    virtual ~ICredentialRepository() = default;
    virtual void CreateCredential(const Credential& cred) = 0;
    virtual std::optional<Credential> GetByUserId(const boost::uuids::uuid& userId) = 0;
    virtual void UpdateCredential(const Credential& cred) = 0;
    virtual void DeleteCredential(const boost::uuids::uuid& userId) = 0;
};
