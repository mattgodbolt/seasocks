file(GLOB_RECURSE SEASOCKS_SOURCE_FILES . *.cpp *.h)

add_library(seasocks STATIC ${SEASOCKS_SOURCE_FILES})
target_compile_definitions(seasocks PRIVATE SEASOCKS_VERSION_STRING=\"${PROJECT_VERSION}\")
target_include_directories(seasocks PUBLIC .)
target_link_libraries(seasocks PRIVATE ${CMAKE_THREAD_LIBS_INIT} embedded)
set_property(TARGET seasocks PROPERTY POSITION_INDEPENDENT_CODE TRUE)

add_library(seasocks_so SHARED)
set_target_properties(seasocks_so PROPERTIES OUTPUT_NAME seasocks)
target_link_libraries(seasocks_so
  PRIVATE 
    "-Wl,--whole-archive"
    seasocks
    "-Wl,--no-whole-archive"
)

install(TARGETS seasocks seasocks_so
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
install(DIRECTORY seasocks
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
        )
