#pragma once
#include <string>

class IPasswordHasher {
public:
    virtual ~IPasswordHasher() = default;

    virtual std::string Hash(const std::string& password, const std::string& pepper) = 0;
    virtual bool Verify(const std::string& hash, const std::string& password, const std::string& pepper = "") = 0;
};
