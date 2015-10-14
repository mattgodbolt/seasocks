
macro(add_app _NAME})
    add_executable(${ARGV0} ${ARGV0}.cpp)
    target_link_libraries(${ARGV0} seasocks)
endmacro()


add_app(ph_test)
add_app(serve)
add_app(ws_echo)
add_app(ws_test)
add_app(ws_test_poll)

