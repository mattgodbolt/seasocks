set(SCRIPT "${PROJECT_SOURCE_DIR}/scripts/gen_embedded.py")


set(EMBEDDED_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/_404.png
    ${CMAKE_CURRENT_SOURCE_DIR}/_error.css
    ${CMAKE_CURRENT_SOURCE_DIR}/_error.html
    ${CMAKE_CURRENT_SOURCE_DIR}/favicon.ico
    ${CMAKE_CURRENT_SOURCE_DIR}/_jquery.min.js
    ${CMAKE_CURRENT_SOURCE_DIR}/_seasocks.css
    ${CMAKE_CURRENT_SOURCE_DIR}/_stats.html
)


add_custom_command(OUTPUT Embedded.cpp
                        COMMAND ${PYTHON_BIN} ${SCRIPT} -o Embedded.cpp -f ${EMBEDDED_FILES}
                        COMMENT "Generating embedded content"
                        )
add_library(embedded OBJECT Embedded.cpp)
target_include_directories(embedded PUBLIC $<TARGET_PROPERTY:seasocks,INTERFACE_INCLUDE_DIRECTORIES>)
set_property(TARGET embedded PROPERTY POSITION_INDEPENDENT_CODE TRUE)
