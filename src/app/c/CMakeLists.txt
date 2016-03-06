macro(add_app _NAME})
    add_executable(${ARGV0} ${ARGV0}.cpp)
    target_link_libraries(${ARGV0} seasocks ${CMAKE_THREAD_LIBS_INIT})
endmacro()

add_app(ph_test)
add_app(serve)
add_app(ws_echo)
add_app(ws_test)
add_app(ws_test_poll)
add_app(async_test)

add_custom_command(TARGET ws_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/src/ws_test_web $<TARGET_FILE_DIR:ws_test>/src/ws_test_web)

add_custom_command(TARGET async_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/src/async_test_web $<TARGET_FILE_DIR:ws_test>/src/async_test_web)
