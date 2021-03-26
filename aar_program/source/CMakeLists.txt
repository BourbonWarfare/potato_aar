file(GLOB SOURCES CONFIGURE_DEPENDS "*.c" "*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/../include/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/../include/*.hpp")
add_executable( ${PROJECT_NAME} ${SOURCES} ${GLOBAL_SOURCES})

