add_subdirectory(main/c)
add_subdirectory(main/web)

macro(add_seasocks_executable _name)
    add_executable(${_name} ${ARGN})
    target_link_libraries(${_name} PRIVATE seasocks)
endmacro()

if (SEASOCKS_EXAMPLE_APP)
    add_subdirectory(app/c)
endif ()

if (UNITTESTS)
    find_program(CMAKE_MEMORYCHECK_COMMAND valgrind)
    enable_testing()
    add_subdirectory(test/c)
endif ()
