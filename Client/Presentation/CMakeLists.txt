add_executable(${PROJECT_NAME} main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE ../Infrastructure/parser_lib)
target_link_libraries(${PROJECT_NAME} PRIVATE parser)

target_include_directories(${PROJECT_NAME} PRIVATE ../Infrastructure/text_lib)
target_link_libraries(${PROJECT_NAME} PRIVATE text_parser)

target_include_directories(${PROJECT_NAME} PRIVATE ../Infrastructure/messenger)
target_link_libraries(${PROJECT_NAME} PRIVATE connection)