macro(add_app NAME)
    add_seasocks_executable(${NAME} ${NAME}.cpp)
endmacro()

add_app(ph_test)
add_app(serve)
add_app(ws_chatroom)
add_app(ws_echo)
add_app(ws_test)
add_app(ws_test_poll)
add_app(async_test)
add_app(streaming_test)

function(copy_directory_post_build TARGET SUBDIRECTORY)
    set(name)
    if (TARGET ${TARGET})
        set(name ${TARGET})
    elseif(TARGET ${TARGET}_so)
        set(name ${TARGET}_so)
    endif()
    if (name)
        add_custom_command(TARGET ${name} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_directory
                "${PROJECT_SOURCE_DIR}/${SUBDIRECTORY}" "$<TARGET_FILE_DIR:${name}>/${SUBDIRECTORY}"
        )
    endif ()
endfunction()

copy_directory_post_build(ws_test src/ws_test_web)
copy_directory_post_build(async_test src/async_test_web)
copy_directory_post_build(ws_chatroom src/ws_chatroom_web)
