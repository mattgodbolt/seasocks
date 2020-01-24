cmake_minimum_required(VERSION 3.3)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(Seasocks VERSION 1.4.3)


option(UNITTESTS "Build unittests." ON)
option(COVERAGE "Build with code coverage enabled" OFF)
option(SEASOCKS_EXAMPLE_APP "Build the example applications." ON) 
option(DEFLATE_SUPPORT "Include support for deflate (requires zlib)." ON)

if (DEFLATE_SUPPORT)
    set(DEFLATE_SUPPORT_BOOL "true")
else ()
    set(DEFLATE_SUPPORT_BOOL "false")
endif ()
message(STATUS "${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "Unittests: ${UNITTESTS}")
message(STATUS "Coverage: ${COVERAGE}")


set(MEMORYCHECK_SUPPRESSIONS_FILE "${PROJECT_SOURCE_DIR}/src/test/suppressions.txt" CACHE INTERNAL "")
include(CTest)
include(Compiler)
include(GNUInstallDirs)
include(ClangFormat)


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.h.in internal/Config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_compile_options(-Wall -Werror -Wextra -pedantic)
if (COVERAGE)
    add_compile_options(--coverage -O0)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif ()

find_package(Threads)


find_program(PYTHON_BIN NAMES python python3 DOC "Python executable")

if (NOT PYTHON_BIN)
    message(SEND_ERROR "Python not found")
else()
    message(STATUS "Using Python: '${PYTHON_BIN}'")
endif ()

if (DEFLATE_SUPPORT)
    find_package(ZLIB REQUIRED)
endif ()

add_subdirectory("src")

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}/
)

