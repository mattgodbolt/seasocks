set(SCRIPT "${CMAKE_SOURCE_DIR}/scripts/gen_embedded.py")
set(PYTHON "python2")

file(GLOB EMBEDDED_FILES "src/main/web/*")

add_custom_command(OUTPUT Embedded.cpp
                        COMMAND ${PYTHON} ${SCRIPT} ${EMBEDDED_FILES} $<ANGLE-R> Embedded.cpp
                        COMMENT "Generating embedded content"
                        )

add_library(embedded OBJECT Embedded.cpp)
