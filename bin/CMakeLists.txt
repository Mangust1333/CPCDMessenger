add_executable(${PROJECT_NAME} main.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/parser_lib)
target_link_libraries(${PROJECT_NAME} PRIVATE parser)

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/text_lib)
target_link_libraries(${PROJECT_NAME} PRIVATE text_parser)

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/messenger)
target_link_libraries(${PROJECT_NAME} PRIVATE connection)