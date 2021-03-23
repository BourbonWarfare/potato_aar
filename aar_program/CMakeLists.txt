project (potato_aar)

add_subdirectory(source)
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/external/asio/asio/include")
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/external/spdlog/include")

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

