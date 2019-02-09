set(SEASOCKS_SOURCE_FILES
        Connection.cpp
        HybiAccept.cpp
        HybiPacketDecoder.cpp
        internal/Base64.cpp
        internal/Base64.h
        internal/ConcreteResponse.h
        internal/Debug.h
        internal/Embedded.h
        internal/HeaderMap.h
        internal/HybiAccept.h
        internal/HybiPacketDecoder.h
        internal/LogStream.h
        internal/PageRequest.h
        Logger.cpp
        md5/md5.cpp
        md5/md5.h
        PageRequest.cpp
        Response.cpp
        seasocks/Connection.h
        seasocks/Credentials.h
        seasocks/IgnoringLogger.h
        seasocks/Logger.h
        seasocks/PageHandler.h
        seasocks/PrintfLogger.h
        seasocks/Request.cpp
        seasocks/Request.h
        seasocks/ResponseBuilder.cpp
        seasocks/ResponseBuilder.h
        seasocks/ResponseCode.cpp
        seasocks/ResponseCodeDefs.h
        seasocks/ResponseCode.h
        seasocks/Response.h
        seasocks/ResponseWriter.h
        seasocks/Server.h
        seasocks/ServerImpl.h
        seasocks/SimpleResponse.h
        seasocks/StreamingResponse.cpp
        seasocks/StreamingResponse.h
        seasocks/StringUtil.h
        seasocks/SynchronousResponse.cpp
        seasocks/SynchronousResponse.h
        seasocks/ToString.h
        seasocks/TransferEncoding.h
        seasocks/util/CrackedUri.h
        seasocks/util/CrackedUriPageHandler.h
        seasocks/util/Html.h
        seasocks/util/Json.h
        seasocks/util/PathHandler.h
        seasocks/util/RootPageHandler.h
        seasocks/util/StaticResponseHandler.h
        seasocks/WebSocket.h
        seasocks/ZlibContext.h
        Server.cpp
        sha1/sha1.cpp
        sha1/sha1.h
        StringUtil.cpp
        util/CrackedUri.cpp
        util/Json.cpp
        util/PathHandler.cpp
        util/RootPageHandler.cpp
        )

if (DEFLATE_SUPPORT)
    set(SEASOCKS_SOURCE_FILES ${SEASOCKS_SOURCE_FILES} seasocks/ZlibContext.cpp)
else()
    set(SEASOCKS_SOURCE_FILES ${SEASOCKS_SOURCE_FILES} seasocks/ZlibContextDisabled.cpp)
endif()

add_library(seasocks_obj OBJECT ${SEASOCKS_SOURCE_FILES})
target_include_directories(seasocks_obj PUBLIC
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/.>
)
set_property(TARGET seasocks_obj PROPERTY POSITION_INDEPENDENT_CODE TRUE)

add_library(seasocks STATIC $<TARGET_OBJECTS:seasocks_obj> $<TARGET_OBJECTS:embedded>)
add_library(Seasocks::seasocks ALIAS seasocks)
target_include_directories(seasocks PUBLIC
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/.>
)
target_link_libraries(seasocks PRIVATE ${CMAKE_THREAD_LIBS_INIT})
if (DEFLATE_SUPPORT)
    target_link_libraries(seasocks PRIVATE "${ZLIB_LIBRARIES}")
endif()

add_library(seasocks_so SHARED $<TARGET_OBJECTS:seasocks_obj> $<TARGET_OBJECTS:embedded>)
add_library(Seasocks::seasocks_so ALIAS seasocks_so)
target_include_directories(seasocks_so PUBLIC ${ZLIB_INCLUDE_DIRS} 
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/.>
)
if (DEFLATE_SUPPORT)
    target_link_libraries(seasocks_so PRIVATE ${CMAKE_THREAD_LIBS_INIT} "${ZLIB_LIBRARIES}")
endif()
set_target_properties(seasocks_so PROPERTIES OUTPUT_NAME seasocks VERSION ${PROJECT_VERSION})

install(TARGETS seasocks seasocks_so EXPORT ${PROJECT_NAME}Config
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
install(DIRECTORY seasocks
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
        )
# export for build tree.
export(
    TARGETS seasocks seasocks_so
    NAMESPACE ${PROJECT_NAME}::
    FILE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
)
# export for install.
install(
  EXPORT ${PROJECT_NAME}Config
  FILE ${PROJECT_NAME}Config.cmake
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}/
)
