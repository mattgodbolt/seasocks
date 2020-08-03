add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE "catch")

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
    StringUtilTests.cpp
    RequestTest.cpp
)

add_test(NAME AllTests COMMAND AllTests)
target_link_libraries(AllTests PRIVATE seasocks Catch)

add_custom_target(unittest AllTests
    COMMENT "Running unittests\n\n"
    VERBATIM
)
