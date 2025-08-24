add_library(
        connection
        Message/Message.h
        Crypto/Crypto.h
        Connection/connection_lib.h
        Connection/connection_lib.cpp
)
target_link_libraries(connection PRIVATE OpenSSL::SSL OpenSSL::Crypto Boost::system Boost::asio nlohmann_json::nlohmann_json)
