Include(FetchContent)

FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.11.0
)

FetchContent_MakeAvailable(Catch2)

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
