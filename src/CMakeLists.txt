
add_subdirectory(main/c)

if(SEASOCKS_SHARED)
    add_subdirectory(main/web)

    if (SEASOCKS_EXAMPLE_APP)
      add_subdirectory(app/c)
    endif ()
endif()

if (UNITTESTS)
    find_program(CMAKE_MEMORYCHECK_COMMAND valgrind)
    enable_testing()
    add_subdirectory(test/c)
endif ()

