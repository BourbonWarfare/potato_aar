file(GLOB SOURCES CONFIGURE_DEPENDS "*.c" "*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/../include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/../include/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/../include/imgui/*.h")
add_executable( ${PROJECT_NAME} ${SOURCES} ${GLOBAL_SOURCES})

target_link_libraries(${PROJECT_NAME} ${GLFW3_LIBRARY})