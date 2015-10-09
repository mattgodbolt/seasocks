cmake_minimum_required(VERSION 3.0)

project(Seasocks VERSION 1.1.6)


set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(EmbeddedContent)
include(CompilerCheck)

configure_file(${CMAKE_MODULE_PATH}/Config.h.in internal/Config.h)

add_definitions(-Wall -std=c++11)

include_directories("src/main/c"
                    ${CMAKE_BINARY_DIR}
                    )

add_subdirectory("src/main/c")
