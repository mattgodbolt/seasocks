add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE "catch")

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

seasocks_link_libraries(AllTests PRIVATE Catch)

if (TARGET AllTests)
    add_test(NAME AllTests COMMAND AllTests)
endif ()
if (TARGET AllTests_so)
    add_test(NAME AllTests_so COMMAND AllTests_so)
endif ()
