cmake_minimum_required(VERSION 2.8.11)

project(Seasocks)
set(PROJECT_VERSION 1.2.3)

message(STATUS "${PROJECT_NAME} ${PROJECT_VERSION}")

option(UNITTESTS "Build unittests." ON)
message(STATUS "Unittests: ${UNITTESTS}")


set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(CTest)
include(EmbeddedContent)
include(CompilerCheck)
include(GNUInstallDirs)


configure_file(${CMAKE_MODULE_PATH}/Config.h.in internal/Config.h)

add_definitions(-Wall -Werror -Wextra -pedantic -ggdb3 -fPIC
        -DSEASOCKS_VERSION_STRING=\"${PROJECT_VERSION}\"
        )

include_directories("src/main/c"
        ${CMAKE_BINARY_DIR}
        )

find_package(Threads)

add_subdirectory("src/main/c")
add_subdirectory("src/app/c")

if (UNITTESTS)
    find_program(MEMORYCHECK_COMMAND valgrind)
    enable_testing()
    add_subdirectory("src/test/c")
endif ()

