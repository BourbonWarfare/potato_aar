cmake_minimum_required(VERSION 3.2.0)
project (potato_aar_extension)
set(PROJECT_TEST_NAME ${PROJECT_NAME}_tests)

file (GLOB SOURCES "*.cpp")
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/aar_extension_tests.cpp")

add_library(${PROJECT_NAME} SHARED ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/external_libraries/asio/asio/include")
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/external_libraries/spdlog/include")
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/external_libraries/bw_common")

add_executable(${PROJECT_TEST_NAME} "aar_extension_tests.cpp" ${GLOBAL_SOURCES})
target_include_directories(${PROJECT_TEST_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/external_libraries/asio/asio/include")
target_include_directories(${PROJECT_TEST_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/external_libraries/spdlog/include")
target_include_directories(${PROJECT_TEST_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/external_libraries/bw_common")

target_link_libraries(${PROJECT_TEST_NAME} PUBLIC ${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
target_compile_definitions(${PROJECT_NAME} PRIVATE -DCOMPILING_EXTENSION)

set_target_properties(${PROJECT_TEST_NAME} PROPERTIES CXX_STANDARD 17)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_SOURCE_DIR}/../${PROJECT_NAME}_x64.dll
)
