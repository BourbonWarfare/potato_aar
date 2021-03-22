cmake_minimum_required(VERSION 3.2.0)
project (potato_aar)

add_subdirectory(source)
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/asio/asio/include")

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

