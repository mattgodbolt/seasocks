cmake_minimum_required(VERSION 2.8.11)

project(Seasocks)
# TODO: Version

add_definitions(-std=c++11)

################################################################################
set(WEB_DIR "${CMAKE_SOURCE_DIR}/src/main/web")
set(EMBEDDED_CONTENT_FILE "${CMAKE_BINARY_DIR}/Embedded.cpp")
set(SCRIPT "${CMAKE_SOURCE_DIR}/scripts/gen_embedded.py")
set(PYTHON "python2") ## TODO Find correct Python


file(GLOB _EMBEDDED_FILES "${WEB_DIR}/*")
list(APPEND _EMBEDDED_FILES "${CMAKE_SOURCE_DIR}/src/main/c/internal/Embedded.h")

#string(REPLACE ";" " " EMBEDDED_FILES "${_EMBEDDED_FILES}") ### XXX: Obsolete?




add_custom_command(OUTPUT Embedded.cpp
    COMMAND ${PYTHON} ${SCRIPT} ${_EMBEDDED_FILES} $<ANGLE-R> Embedded.cpp

BYPRODUCTS Embedded.cpp

COMMENT "Generate embedded content"
)

add_custom_target(embedded ALL
    DEPENDS Embedded.cpp)




#add_custom_command(OUTPUT output1 [output2 ...]
#                   COMMAND command1 [ARGS] [args1...]
#                   [COMMAND command2 [ARGS] [args2...] ...]
#                   [MAIN_DEPENDENCY depend]
#                   [DEPENDS [depends...]]
#                   [BYPRODUCTS [files...]]
#                   [IMPLICIT_DEPENDS <lang1> depend1
#                                    [<lang2> depend2] ...]
#                   [WORKING_DIRECTORY dir]
#                   [COMMENT comment]
#                   [VERBATIM] [APPEND] [USES_TERMINAL])


#execute_process(COMMAND 
#    ${PYTHON} ${SCRIPT} ${_EMBEDDED_FILES}
#    RESULT_VARIABLE _R
##    OUTPUT_VARIABLE _O
#    OUTPUT_FILE "${EMBEDDED_CONTENT_FILE}"
#    )

#file(WRITE ${EMBEDDED_CONTENT_FILE} "${_O}")

################################################################################


include_directories("src/main/c")
add_subdirectory("src/main/c")


#EMBEDDED_CONTENT:=$(shell find src/main/web -type f)
#$(OBJ_DIR)/embedded.o: scripts/gen_embedded.py $(EMBEDDED_CONTENT) src/main/c/internal/Embedded.h
