
add_subdirectory("main/c")
add_subdirectory("main/web")
add_subdirectory("app/c")

if (UNITTESTS)
    find_program(CMAKE_MEMORYCHECK_COMMAND valgrind)
    enable_testing()
    add_subdirectory("test/c")
endif ()

