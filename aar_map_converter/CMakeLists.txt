project (potato_aar_map_converter)

add_subdirectory(source)
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

