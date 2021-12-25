cmake_minimum_required(VERSION 3.2.0)
project(potato_aar_main)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

add_subdirectory(extensions)
add_subdirectory(aar_program)
add_subdirectory(aar_map_converter)

