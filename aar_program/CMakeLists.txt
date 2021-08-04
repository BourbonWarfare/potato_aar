project (potato_aar)

find_package(GLFW3 REQUIRED)
find_package(OpenGL 3 REQUIRED)

add_subdirectory(source)
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/external_libraries/asio/asio/include")
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/external_libraries/spdlog/include")
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/external_libraries/bw_common")
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/external_libraries/imgui")
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/external_libraries/nlohmann")
target_include_directories(${PROJECT_NAME} PUBLIC ${GLFW3_INCLUDE_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

