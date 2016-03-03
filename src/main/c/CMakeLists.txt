add_library(seasocks
        md5/md5.cpp
        md5/md5.h
        Connection.cpp
        Server.cpp
        util/RootPageHandler.cpp
        util/PathHandler.cpp
        util/Json.cpp
        util/CrackedUri.cpp
        StringUtil.cpp
        Response.cpp
        sha1/sha1.cpp
        sha1/sha1.h
        HybiPacketDecoder.cpp
        PageRequest.cpp
        Logger.cpp
        HybiAccept.cpp
        seasocks/ServerImpl.h
        seasocks/Server.h
        seasocks/ToString.h
        seasocks/ResponseBuilder.h
        seasocks/Request.h
        seasocks/IgnoringLogger.h
        seasocks/Credentials.h
        seasocks/PrintfLogger.h
        seasocks/util/PathHandler.h
        seasocks/util/CrackedUri.h
        seasocks/util/StaticResponseHandler.h
        seasocks/util/CrackedUriPageHandler.h
        seasocks/util/Html.h
        seasocks/util/RootPageHandler.h
        seasocks/util/Json.h
        seasocks/Response.h
        seasocks/Logger.h
        seasocks/ResponseCodeDefs.h
        seasocks/ResponseBuilder.cpp
        seasocks/ResponseCode.h
        seasocks/ResponseCode.cpp
        seasocks/PageHandler.h
        seasocks/Request.cpp
        seasocks/StringUtil.h
        seasocks/Connection.h
        seasocks/WebSocket.h
        internal/HeaderMap.h
        internal/HybiAccept.h
        internal/Base64.h
        internal/Base64.cpp
        internal/Config.h
        internal/Embedded.h
        internal/HybiPacketDecoder.h
        internal/Debug.h
        internal/LogStream.h
        internal/Version.h
        internal/PageRequest.h
        internal/ConcreteResponse.h
        seasocks/ResponseWriter.h
        seasocks/SynchronousResponse.cpp
        seasocks/SynchronousResponse.h

        $<TARGET_OBJECTS:embedded>
        )

install(TARGETS seasocks
                LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
                )
install(DIRECTORY seasocks
                    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
                    FILES_MATCHING PATTERN "*.h"
                    )

