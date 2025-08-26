cmake_minimum_required(VERSION 3.27)

project(
        MessengerServer
        VERSION 0.0
        DESCRIPTION "This is server"
        HOMEPAGE_URL "coming soon"
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)

FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
)

FetchContent_MakeAvailable(nlohmann_json)

find_package(Boost REQUIRED COMPONENTS system asio uuid)
find_package(jwt-cpp)

add_subdirectory(BusinessLayer)
add_subdirectory(Infrastructure)
add_subdirectory(Presentation)

target_link_libraries(MessengerServer PRIVATE Boost::system Boost::asio Boost::uuid jwt-cpp::jwt-cpp nlohmann_json::nlohmann_json)

if(WIN32)
    target_link_libraries(MessengerServer PRIVATE ws2_32 mswsock)
endif()