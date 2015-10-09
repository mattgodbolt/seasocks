
#if( NOT EXISTS ${EMBEDDED_CONTENT_FILE} )
#    message(SEND_ERROR "No embedded content file  found: '${EMBEDDED_CONTENT_FILE}'")
#endif()

add_library(seasocks ./HybiAccept.cpp
    ./StringUtil.cpp
    ./Server.cpp
    ./internal/Base64.cpp
    ./Connection.cpp
    ./HybiPacketDecoder.cpp
    ./PageRequest.cpp
    ./Logger.cpp
    ./Response.cpp
    ./md5/md5.cpp
    ./util/CrackedUri.cpp
    ./util/Json.cpp
    ./util/PathHandler.cpp
    ./util/RootPageHandler.cpp
    ./sha1/sha1.cpp
    ./seasocks/ResponseCode.cpp
    ./seasocks/ResponseBuilder.cpp
    ./seasocks/Request.cpp

    ${EMBEDDED_CONTENT_FILE}

        )

add_dependencies(seasocks embedded)
