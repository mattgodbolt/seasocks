
set(TESTS "AllTests")

add_executable(AllTests test_main.cpp
                        ConnectionTests.cpp 
                        CrackedUriTests.cpp
                        HeaderMapTests.cpp
                        HtmlTests.cpp
                        HybiTests.cpp
                        JsonTests.cpp
                        MockServerImpl.h
                        )

add_test(NAME ${TESTS} COMMAND ${TESTS})
target_include_directories(${TESTS} PUBLIC ${gtest_INCLUDE_DIRS}
                                            ${gmock_INCLUDE_DIRS}
                                            ${CMAKE_CURRENT_SOURCE_DIR}
                                            )
target_link_libraries(${TESTS} ${gtest_LIBRARIES} 
                                ${gmock_LIBRARIES}
                                ${CMAKE_THREAD_LIBS_INIT}
                                seasocks
                                )
