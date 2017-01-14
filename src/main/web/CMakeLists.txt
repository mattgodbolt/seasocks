set(SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_embedded.py")
set(PYTHON "python2")

set(EMBEDDED_FILES 
  _404.png
  _error.css
  _error.html
  favicon.ico
  _jquery.min.js
  _seasocks.css
  _stats.html
)

file(GLOB EMBEDDED_FILES "src/main/web/*")

add_custom_command(OUTPUT Embedded.cpp
                        COMMAND ${PYTHON} ${SCRIPT} ${EMBEDDED_FILES} $<ANGLE-R> Embedded.cpp
                        COMMENT "Generating embedded content"
                        )

add_library(embedded STATIC Embedded.cpp)
set_property(TARGET embedded PROPERTY POSITION_INDEPENDENT_CODE TRUE)
target_include_directories(embedded 
  PUBLIC 
    .
)
