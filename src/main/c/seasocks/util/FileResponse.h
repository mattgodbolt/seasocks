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

#pragma once

#include <memory>

#include "seasocks/Response.h"
#include "seasocks/ResponseWriter.h"
#include "seasocks/Request.h"
#include "seasocks/util/PathHandler.h"

namespace seasocks {

class FileResponse : public Response
{
public:
    /// Supports returning file data back to a client supporting deflate compression on the fly.
    /// If compression is disabled, file resuming is supported.
    /// If compression is enabled, files resumption is disabled and file size is not provided to the client.
    /// These are only determinable if the file is compressed before transmission, which this Response does not do.
    /// @param request            Incoming request
    /// @param filePath           Path to file to return to client in filesystem
    /// @param contentType        Content-Type header value for file
    /// @param allowCompression   Whether to support compressing the file on the fly with deflate during transmission (disables resuming and content length)
    /// @param allowCaching       Whether the file contents are allowed to be cached
    explicit FileResponse(const Request &request, const std::string &filePath, const std::string &contentType, bool allowCompression, bool allowCaching);
    
    virtual void handle(std::shared_ptr<ResponseWriter> writer) override;
    
    /// Same as handle but does not spawn a thread
    void respond(std::shared_ptr<ResponseWriter> writer);
    
    virtual void cancel() override;
    
    
private:
    
    ResponseCode parseHeaders(const off_t fileSize, const time_t fileLastModified, bool sendCompressedData, off_t& fileTransferStart, off_t& fileTransferEnd) const;
    
    const Request &_request;
    std::string _path;
    std::string _contentType;
    bool _allowCompression;
    bool _allowCaching;
    bool _cancelled;
};

}
