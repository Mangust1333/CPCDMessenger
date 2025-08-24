cmake_minimum_required(VERSION 3.27)

project(
        Messenger
        VERSION 0.0
        DESCRIPTION "This is mine project, i like it"
        HOMEPAGE_URL "coming soon"
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)

find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system asio)

include(FetchContent)

FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
)

FetchContent_MakeAvailable(nlohmann_json)

add_subdirectory(text_lib)
add_subdirectory(parser_lib)
add_subdirectory(messenger)
add_subdirectory(bin)

target_link_libraries(Messenger PRIVATE OpenSSL::SSL OpenSSL::Crypto Boost::system Boost::asio nlohmann_json::nlohmann_json)

if(WIN32)
    target_link_libraries(Messenger PRIVATE ws2_32 mswsock)
endif()