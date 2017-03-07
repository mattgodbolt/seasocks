set(SEASOCKS_SOURCE_FILES Connection.cpp
                        HybiAccept.cpp
                        HybiPacketDecoder.cpp
                        Logger.cpp
                        PageRequest.cpp
                        Response.cpp
                        Server.cpp
                        StringUtil.cpp
                        md5/md5.cpp
                        internal/Base64.cpp
                        seasocks/Request.cpp
                        seasocks/ResponseCode.cpp
                        seasocks/SynchronousResponse.cpp
                        seasocks/ResponseBuilder.cpp
                        sha1/sha1.cpp
                        util/CrackedUri.cpp
                        util/Json.cpp
                        util/PathHandler.cpp
                        util/RootPageHandler.cpp
                        )

add_library(seasocks_obj OBJECT ${SEASOCKS_SOURCE_FILES})
target_include_directories(seasocks_obj PUBLIC .)
set_property(TARGET seasocks_obj PROPERTY POSITION_INDEPENDENT_CODE TRUE)

add_library(seasocks STATIC $<TARGET_OBJECTS:seasocks_obj> $<TARGET_OBJECTS:embedded>)
target_include_directories(seasocks PUBLIC .)
target_link_libraries(seasocks PRIVATE ${CMAKE_THREAD_LIBS_INIT})

add_library(seasocks_so SHARED $<TARGET_OBJECTS:seasocks_obj> $<TARGET_OBJECTS:embedded>)
target_include_directories(seasocks_so PUBLIC ${ZLIB_INCLUDE_DIRS} .)
target_link_libraries(seasocks_so PRIVATE ${CMAKE_THREAD_LIBS_INIT} "${ZLIB_LIBRARIES}")
set_target_properties(seasocks_so PROPERTIES OUTPUT_NAME seasocks)

install(TARGETS seasocks seasocks_so
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
install(DIRECTORY seasocks
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
        )
