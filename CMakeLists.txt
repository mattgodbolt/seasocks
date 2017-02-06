cmake_minimum_required(VERSION 3.3)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(Seasocks)
set(PROJECT_VERSION 1.2.6)


option(UNITTESTS "Build unittests." ON)

message(STATUS "${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "Unittests: ${UNITTESTS}")



set(MEMORYCHECK_SUPPRESSIONS_FILE "${PROJECT_SOURCE_DIR}/src/test/suppressions.txt" CACHE INTERNAL "") 
include(CTest)
include(CompilerCheck)
include(GNUInstallDirs)


configure_file(${CMAKE_MODULE_PATH}/Config.h.in internal/Config.h)
include_directories(${CMAKE_BINARY_DIR})

add_definitions(-Wall -Werror -Wextra -pedantic -ggdb3)

find_package(Threads)


find_program(PYTHON_BIN python2 DOC "Python executable")

if( NOT PYTHON_BIN )
    message(SEND_ERROR "Python not found")
endif()


add_subdirectory("src/main/c")
add_subdirectory("src/main/web")
add_subdirectory("src/app/c")

if (UNITTESTS)
    find_program(CMAKE_MEMORYCHECK_COMMAND valgrind)
    enable_testing()
    add_subdirectory("src/test/c")
endif ()

