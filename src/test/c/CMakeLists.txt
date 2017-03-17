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
        ToStringTests.cpp
        EmbeddedContentTests.cpp
        ResponseBuilderTests.cpp
        )

add_test(NAME ${TESTS} COMMAND ${TESTS})
target_link_libraries(${TESTS} seasocks "${ZLIB_LIBRARIES}")
