add_library(
        connection
        ClientSession.h
        Server.h
        ConsoleClient.h
        ClientSession.cpp
        ConsoleClient.cpp
        Server.cpp
        IServer.h
        ISession.h
)
target_link_libraries(connection PRIVATE OpenSSL::SSL OpenSSL::Crypto Boost::system Boost::asio nlohmann_json::nlohmann_json)
