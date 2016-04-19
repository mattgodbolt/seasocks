set(TESTS "AllTests")

add_executable(AllTests test_main.cpp
        catch.hpp
        ConnectionTests.cpp
        CrackedUriTests.cpp
        HeaderMapTests.cpp
        HtmlTests.cpp
        HybiTests.cpp
        JsonTests.cpp
        MockServerImpl.h
        ServerTests.cpp
        ToStringTests.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
add_test(NAME ${TESTS} COMMAND ${TESTS})
target_include_directories(${TESTS} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(${TESTS}
        ${CMAKE_THREAD_LIBS_INIT}
        seasocks
)
