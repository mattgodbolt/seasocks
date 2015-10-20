add_library(seasocks HybiAccept.cpp
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

