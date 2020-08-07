cmake_minimum_required(VERSION 3.3)
project(Seasocks VERSION 1.4.4)

if(WIN32)
    message(FATAL_ERROR "${PROJECT_NAME} does not support Windows")
endif()
if(APPLE)
    message(FATAL_ERROR "${PROJECT_NAME} does not support Apple platforms")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(SEASOCKS_SHARED "Build seasocks as a shared library" "${BUILD_SHARED_LIBS}")
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
message(STATUS "Building shared: ${SEASOCKS_SHARED}")


set(MEMORYCHECK_SUPPRESSIONS_FILE "${PROJECT_SOURCE_DIR}/src/test/suppressions.txt" CACHE INTERNAL "")
include(CTest)
include(GNUInstallDirs)
include(ClangFormat)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.h.in internal/Config.h @ONLY)
add_compile_options(-Wall -Werror -Wextra -pedantic -pedantic-errors)
if (COVERAGE)
    add_compile_options(--coverage -O0)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    set(CMAKE_SHARED_LINKER_FLAGS "${SHARED_EXE_LINKER_FLAGS} --coverage")
endif ()

find_package(Threads)
find_package(PythonInterp)

if (NOT PYTHONINTERP_FOUND)
    message(SEND_ERROR "Python not found")
else()
    message(STATUS "Using Python: '${PYTHON_EXECUTABLE}'")
endif ()

if (DEFLATE_SUPPORT)
    find_package(ZLIB REQUIRED)
endif ()

add_subdirectory(src)

include(CMakePackageConfigHelpers)
configure_package_config_file(cmake/SeasocksConfig.cmake.in ${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}/"
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}/
)
install(FILES
    LICENSE
    DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/licenses/${PROJECT_NAME}"
)
