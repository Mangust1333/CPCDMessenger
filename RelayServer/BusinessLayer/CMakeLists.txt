add_library(
        RelayServerCore
        domain/User.h
        services/UserService/UserService.cpp
        services/UserService/UserService.h
        domain/Message.h
        domain/Chat.h
        domain/ChatType.h
        services/AuthService/JwtAuthService.cpp
        services/AuthService/JwtAuthService.h
        services/AuthService/IAuthService.h
        domain/Session.h
        repository/IUserRepository.h
        repository/IFriendRepository.h
        domain/FriendRelation.h
        services/UserService/IUserService.h
        services/UserService/UserServerErrors/CreateUserError.h
        services/UserService/UserServerErrors/UpdateUserError.h
        services/UserService/UserServerErrors/AddFriendError.h
        repository/ICredentialRepository.h
        domain/Credential.h
        contracts/IPasswordHasher.h
)

target_link_libraries(RelayServerCore PRIVATE Boost::uuid jwt-cpp::jwt-cpp nlohmann_json::nlohmann_json)