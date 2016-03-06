macro(add_app _NAME)
    add_executable(${_NAME} ${_NAME}.cpp)
    target_link_libraries(${_NAME} seasocks)
endmacro()

add_app(ph_test)
add_app(serve)
add_app(ws_echo)
add_app(ws_test)
add_app(ws_test_poll)

add_custom_command(TARGET ws_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/src/ws_test_web $<TARGET_FILE_DIR:ws_test>/src/ws_test_web)
