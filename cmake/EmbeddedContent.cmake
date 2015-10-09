

set(SCRIPT "${CMAKE_SOURCE_DIR}/scripts/gen_embedded.py")
set(PYTHON "python2") ## TODO Find correct Python


file(GLOB EMBEDDED_FILES "src/main/web/*")
list(APPEND EMBEDDED_FILES "${CMAKE_SOURCE_DIR}/src/main/c/internal/Embedded.h")



add_custom_command(OUTPUT Embedded.cpp
                        COMMAND ${PYTHON} ${SCRIPT} ${EMBEDDED_FILES} $<ANGLE-R> Embedded.cpp
                        BYPRODUCTS Embedded.cpp
                        COMMENT "Generate embedded content"
                        )

add_custom_target(embedded ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Embedded.cpp)
