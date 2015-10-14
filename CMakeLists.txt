cmake_minimum_required(VERSION 2.8.11)

project(Seasocks)
set(PROJECT_VERSION 1.1.6)

message(STATUS "${PROJECT_NAME} ${PROJECT_VERSION}")

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(EmbeddedContent)
include(CompilerCheck)

include(GNUInstallDirs)

configure_file(${CMAKE_MODULE_PATH}/Config.h.in internal/Config.h)

add_definitions(-Wall -std=c++11 -DSEASOCKS_VERSION_STRING=\"${PROJECT_VERSION}\")

include_directories("src/main/c"
                    ${CMAKE_BINARY_DIR}
                    )

add_subdirectory("src/main/c")
add_subdirectory("src/app/c")
