cmake_minimum_required(VERSION 3.0)

project(Seasocks VERSION 1.1.6)


set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(EmbeddedContent)



add_definitions(-std=c++11)
include_directories("src/main/c")

add_subdirectory("src/main/c")
