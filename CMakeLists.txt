cmake_minimum_required(VERSION 3.27)

project(
        Messenger
        VERSION 0.0
        DESCRIPTION "This is mine project, i like it"
        HOMEPAGE_URL "coming soon"
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(text_lib)
add_subdirectory(parser_lib)
add_subdirectory(bin)

if (WIN32)
    target_link_libraries(Messenger PRIVATE ws2_32)
endif()