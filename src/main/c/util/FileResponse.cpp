// Copyright (c) 2018, Joe Balough
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:
// 
// Redistributions of source code must retain the above copyright notice, this 
// list of conditions and the following disclaimer.
// 
// Redistributions in binary form must reproduce the above copyright notice, 
// this list of conditions and the following disclaimer in the documentation 
// and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
// POSSIBILITY OF SUCH DAMAGE.

#include "seasocks/util/FileResponse.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <regex>
#include <thread>

#include "seasocks/Server.h"
#include "seasocks/StringUtil.h"
#include "seasocks/ToString.h"
#include "seasocks/ZlibContext.h"

using namespace seasocks;

constexpr size_t ReadWriteBufferSize = 16 * 1024;

FileResponse::FileResponse(const Request &request, const std::string &filePath, const std::string &contentType, bool allowCompression, bool allowCaching) :
        _request(request), _path(filePath), _contentType(contentType), _allowCompression(allowCompression), _allowCaching(allowCaching), _cancelled(false)
{
}

void FileResponse::handle(std::shared_ptr<seasocks::ResponseWriter> writer)
{
    // Launch a thread to actually handle the request, capture this object and the writer object
    std::thread t([this, writer] () mutable
    {
        respond(writer);
    });
    t.detach();
}

void seasocks::FileResponse::respond(std::shared_ptr<ResponseWriter> writer)
{
    Server &server = _request.server();
    ResponseCode requestResponse = ResponseCode::Ok;
    std::string contentType = _contentType;
    bool cacheable = _allowCaching;
    bool sendCompressedData = false;
    ZlibContext zlib;

    int input = ::open(_path.c_str(), O_RDONLY);
    struct stat stat;
    if (input == -1 || ::fstat(input, &stat) == -1) {
        requestResponse = ResponseCode::NotFound;
    }
    
    off_t size = stat.st_size;
    time_t lastModified = stat.st_mtime;
    
    // Starting and ending position in the file to transfer
    // By default, transferring the whole file
    off_t start = 0;
    off_t end = size;
    
    if (requestResponse != ResponseCode::NotFound) {
        // Check if the client supports deflate compression
        if (_request.hasHeader("Accept-Encoding") && _allowCompression) {
            if (_request.getHeader("Accept-Encoding").find("deflate") != std::string::npos) {
                sendCompressedData = true;
            }
        }
        
        // If compression is allowed, try to initialize the zlib context here
        // It will throw a runtime_error if zlib was diabled in the build
        if (sendCompressedData) {
            try {
                zlib.initialise();
            }
            catch (std::runtime_error &e)
            {
                sendCompressedData = false;
            }
        }
        
        // Parse headers for a variety of conditions including range
        // If no range headers were provided, start and end are left unmodified,
        // otherwise, they are set to the start and end bytes requested
        requestResponse = parseHeaders(size, lastModified, sendCompressedData, start, end);
        
        // Make sure the file can be seeked to the requested data
        // This should really never happen so do it early to provide client a 500 if it does
        if (requestResponse == ResponseCode::Ok || requestResponse == ResponseCode::PartialContent) {
            if (::lseek(input, start, SEEK_SET) == -1)
            {
                requestResponse = ResponseCode::InternalServerError;
            }
        }
    }
    
    
    // Write headers back to client
    server.execute([writer, requestResponse, contentType, lastModified, cacheable, sendCompressedData, start, end, size] {
        writer->begin(requestResponse);
        writer->header("Content-Type", contentType);
        writer->header("Connection", "keep-alive");
        writer->header("Last-Modified", webtime(lastModified));
        writer->header("Vary", "Accept-Encoding");
        
        if (sendCompressedData) {
            writer->header("Content-Encoding", "deflate");
        }
        else {
            writer->header("Accept-Ranges", "bytes");
            writer->header("Content-Length", toString(end - start));
            
            if (requestResponse == ResponseCode::RangeNotSatisfiable) {
                // rfc7233 4.2 says Range Not Satisfiable responses should provide a special Content-Range header
                writer->header("Content-Range", "bytes */" + toString(size));
            }
            else {
                writer->header("Content-Range", "bytes " + toString(start) + "-" + toString(end) + "/" + toString(size));
            }
        }
        
        if (!cacheable) {
            writer->header("Cache-Control", "no-store");
            writer->header("Pragma", "no-cache");
            writer->header("Expires", now());
        }
        
        // If the response is reporting some kind of failure, close the connection without writing anything else
        if (requestResponse != ResponseCode::Ok && requestResponse != ResponseCode::PartialContent) {
            writer->finish(false);
        }
    });
    
    if (requestResponse != ResponseCode::Ok && requestResponse != ResponseCode::PartialContent) {
        ::close(input);
        return;
    }
    
    std::vector<uint8_t> compressionOutput;
    compressionOutput.reserve(ReadWriteBufferSize);
    auto bytesLeft = end - start;
    while (bytesLeft && ! _cancelled) {
        uint8_t buf[ReadWriteBufferSize];
        auto bytesRead = ::read(input, buf, std::min( (unsigned long) sizeof(buf), (unsigned long) bytesLeft));
        if (bytesRead < 0) {
            // We can't send an error document as we've sent the header
            break;
        }
        bytesLeft -= bytesRead;
        // compress data if possible
        if (sendCompressedData) {
            zlib.deflate(buf, bytesRead, compressionOutput);
            
            server.execute([writer, compressionOutput] {
                writer->payload(compressionOutput.data(), compressionOutput.size(), true);
            });
            
            compressionOutput.clear();
        }
        else {
            server.execute([writer, buf, bytesRead] {
                writer->payload(buf, bytesRead, true);
            });
            
        }
    }
    
    ::close(input);
    
    // All data written to client, close the connection
    server.execute([writer] {
        writer->finish(false);
    });
}

void FileResponse::cancel()
{
    _cancelled = true;
}

seasocks::ResponseCode FileResponse::parseHeaders(const off_t fileSize, const time_t fileLastModified, bool sendCompressedData, off_t& fileTransferStart, off_t& fileTransferEnd) const {
    // Allowable format: ^bytes=([0-9]+)-([0-9]*)$
    // Do not support non-byte unit, multi-part range requests, or ETags
    
    if (_request.hasHeader("If-Modified-Since")) {
        time_t ifModifiedSince = webtime(_request.getHeader("If-Modified-Since"));
        if (fileLastModified <= ifModifiedSince) {
            return ResponseCode::NotModified;
        }
    }
    
    if (_request.hasHeader("If-Unmodified-Since")) {
        time_t ifUnmodifiedSince = webtime(_request.getHeader("If-Unmodified-Since"));
        if (fileLastModified > ifUnmodifiedSince) {
            return ResponseCode::PreconditionFailed;
        }
    }
    
    if (_request.hasHeader("If-Range") && !sendCompressedData) {
        // If provided If-Range is not parseable, ifRange gets set to -1 which should definitely be less than the last modified time
        time_t ifRange = webtime(_request.getHeader("If-Range"));
        if (ifRange != fileLastModified) {
            return ResponseCode::Ok;
        }
    }
    
    // Handle a Range Request
    if (_request.hasHeader("Range") && !sendCompressedData) {
        std::string rangeString = _request.getHeader("Range");
        
        // Check for multipart range
        std::regex multipartRegex("^\\S+=[0-9]+-[0-9]*, [0-9]+-[0-9]*");
        if ( std::regex_match(rangeString, multipartRegex) ) {
            return ResponseCode::Ok;
        }
        
        // Parse non-multipart range
        std::regex rangeRegex("^bytes=([0-9]+)-([0-9]*)$");
        std::smatch rangeStrings;
        if (! std::regex_match(rangeString, rangeStrings, rangeRegex) ) {
            return ResponseCode::RangeNotSatisfiable;
        }
        
        off_t parsedStart = atoi(rangeStrings[1].str().c_str());
        off_t parsedStop = atoi(rangeStrings[2].str().c_str());
        
        // If no end specified, return data to end of file
        if (rangeStrings[2].str().empty()) {
            parsedStop = fileSize;
        }
        
        // Check that start and stop are valid
        if (parsedStart < 0 || parsedStart > fileSize) {
            return ResponseCode::RangeNotSatisfiable;
        }
        if (parsedStop < parsedStart || parsedStop > fileSize) {
            return ResponseCode::RangeNotSatisfiable;
        }
        
        fileTransferStart = parsedStart;
        fileTransferEnd = parsedStop;
        
        return ResponseCode::PartialContent;
    }
    
    return ResponseCode::Ok;
}
