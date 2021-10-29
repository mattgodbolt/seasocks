if (SEASOCKS_SHARED)
    set(SEASOCKS_LIBTYPE "SHARED")
else ()
    set(SEASOCKS_LIBTYPE "STATIC")
endif ()

if (WIN32)
    set(WIN_FILES
        "${PROJECT_SOURCE_DIR}/win32/wepoll.c"
        "${PROJECT_SOURCE_DIR}/win32/wepoll.h"
        "${PROJECT_SOURCE_DIR}/win32/win_byteswap.h"
        "${PROJECT_SOURCE_DIR}/win32/win_getopt.h"
        "${PROJECT_SOURCE_DIR}/win32/win_unistd.h"
		"${PROJECT_SOURCE_DIR}/win32/winsock_includes.h"
        )
else()
        set (WIN_FILES "")
endif()

add_library(seasocks ${SEASOCKS_LIBTYPE}
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
        ${WIN_FILES}
        $<TARGET_OBJECTS:embedded>
         "seasocks/StrCompare.h")

set_target_properties(
    seasocks
    PROPERTIES
        VERSION ${Seasocks_VERSION}
)

if (DEFLATE_SUPPORT)
    target_sources(seasocks PRIVATE seasocks/ZlibContext.cpp "seasocks/StrCompare.h")
else ()
    target_sources(seasocks PRIVATE seasocks/ZlibContextDisabled.cpp)
endif ()
target_include_directories(seasocks PUBLIC
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/.>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/.>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/.>
        )

add_library(Seasocks::seasocks ALIAS seasocks)
target_link_libraries(seasocks PRIVATE Threads::Threads)
if (DEFLATE_SUPPORT)
    target_link_libraries(seasocks PRIVATE ZLIB::ZLIB)
endif ()
install(TARGETS seasocks EXPORT ${PROJECT_NAME}
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        )

install(DIRECTORY seasocks
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        FILES_MATCHING PATTERN "*.h"
        )

install(FILES "${PROJECT_BINARY_DIR}/internal/Config.h"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/seasocks/internal"
        )

install(
        EXPORT ${PROJECT_NAME}
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}/"
)
