macro(add_app _NAME)
    add_executable(${_NAME} ${_NAME}.cpp)
    target_link_libraries(${_NAME} seasocks "${ZLIB_LIBRARIES}")
endmacro()

add_app(ph_test)
add_app(serve)
add_app(ws_chatroom)
add_app(ws_echo)
add_app(ws_test)
add_app(ws_test_poll)
add_app(async_test)
add_app(streaming_test)

add_custom_command(TARGET ws_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/src/ws_test_web $<TARGET_FILE_DIR:ws_test>/src/ws_test_web)

add_custom_command(TARGET async_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/src/async_test_web $<TARGET_FILE_DIR:async_test>/src/async_test_web)

add_custom_command(TARGET ws_chatroom POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/src/ws_chatroom_web $<TARGET_FILE_DIR:ws_chatroom>/src/ws_chatroom_web)
