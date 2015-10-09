
add_library(seasocks ${CMAKE_BINARY_DIR}/Embedded.cpp
                        HybiAccept.cpp
                        StringUtil.cpp
                        Server.cpp
                        internal/Base64.cpp
                        Connection.cpp
                        HybiPacketDecoder.cpp
                        PageRequest.cpp
                        Logger.cpp
                        Response.cpp
                        md5/md5.cpp
                        util/CrackedUri.cpp
                        util/Json.cpp
                        util/PathHandler.cpp
                        util/RootPageHandler.cpp
                        sha1/sha1.cpp
                        seasocks/ResponseCode.cpp
                        seasocks/ResponseBuilder.cpp
                        seasocks/Request.cpp
                        )

add_dependencies(seasocks embedded)
