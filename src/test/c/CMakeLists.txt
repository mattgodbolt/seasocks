add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE "catch")

set(TESTS "AllTests")

add_executable(AllTests
        test_main.cpp
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
        ResponseTests.cpp
        )

add_test(NAME ${TESTS} COMMAND ${TESTS})
target_link_libraries(${TESTS} PRIVATE seasocks ${ZLIB_LIBRARIES} Catch)

add_custom_target(unittest ${TESTS}
                    COMMENT "Running unittests\n\n"
                    VERBATIM
                    )
