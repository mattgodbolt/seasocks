find_package(Catch2 3 REQUIRED)

add_seasocks_executable(AllTests
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
        StringUtilTests.cpp
        RequestTest.cpp
        )

target_link_libraries(AllTests PRIVATE Catch2::Catch2WithMain Threads::Threads)

add_test(NAME AllTests COMMAND AllTests)
