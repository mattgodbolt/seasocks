

set(SCRIPT "${CMAKE_SOURCE_DIR}/scripts/gen_embedded.py")
set(PYTHON "python2")


file(GLOB EMBEDDED_FILES "src/main/web/*")
list(APPEND EMBEDDED_FILES "${CMAKE_SOURCE_DIR}/src/main/c/internal/Embedded.h")



add_custom_command(OUTPUT Embedded.cpp
                        COMMAND ${PYTHON} ${SCRIPT} ${EMBEDDED_FILES} $<ANGLE-R> Embedded.cpp
                        COMMENT "Generate embedded content"
                        )

add_Library(embedded Embedded.cpp)
