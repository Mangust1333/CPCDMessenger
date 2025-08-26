cmake_minimum_required(VERSION 3.27)

project(
        CPCDMessenger
        VERSION 0.0
        DESCRIPTION "Messenger project with client and server"
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(Client)
add_subdirectory(RelayServer)
