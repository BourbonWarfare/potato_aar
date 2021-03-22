cmake_minimum_required(VERSION 3.2.0)
project (potato_aar)

file (GLOB SOURCES "aar_extension.cpp")

add_subdirectory(cpr)

add_library(potato_aar SHARED ${SOURCES})
include_directories(${CPR_INCLUDE_DIRS})
target_link_libraries(potato_aar ${CPR_LIBRARIES})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_SOURCE_DIR}/../${PROJECT_NAME}_x64.dll
)
