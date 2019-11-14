// Copyright (c) 2013-2017, Matt Godbolt
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

// Not a normal header file - no header guards on purpose.
// Do not directly include! Use ResponseCode.h instead

#ifdef SEASOCKS_DEFINE_RESPONSECODE // workaround for header check

SEASOCKS_DEFINE_RESPONSECODE(100, Continue, "Continue")
SEASOCKS_DEFINE_RESPONSECODE(101, WebSocketProtocolHandshake, "WebSocket Protocol Handshake")
SEASOCKS_DEFINE_RESPONSECODE(102, Processing, "Processing")
SEASOCKS_DEFINE_RESPONSECODE(103, Checkpoint, "Checkpoint")

SEASOCKS_DEFINE_RESPONSECODE(200, Ok, "OK")
SEASOCKS_DEFINE_RESPONSECODE(201, Created, "Created")
SEASOCKS_DEFINE_RESPONSECODE(202, Accepted, "Accepted")
SEASOCKS_DEFINE_RESPONSECODE(203, NonAuthoritativeInformation, "Non Authoritative Information")
SEASOCKS_DEFINE_RESPONSECODE(204, NoContent, "No Content")
SEASOCKS_DEFINE_RESPONSECODE(205, ResetContent, "Reset Content")
SEASOCKS_DEFINE_RESPONSECODE(206, PartialContent, "Partial Content")
SEASOCKS_DEFINE_RESPONSECODE(207, MultiStatus, "Multi-Status")
SEASOCKS_DEFINE_RESPONSECODE(208, AlreadyReported, "Already Reported")
SEASOCKS_DEFINE_RESPONSECODE(226, IMUsed, "IM Used")

SEASOCKS_DEFINE_RESPONSECODE(300, MultipleChoices, "Multiple Choices")
SEASOCKS_DEFINE_RESPONSECODE(301, MovedPermanently, "Moved Permanently")
SEASOCKS_DEFINE_RESPONSECODE(302, Found, "Found")
SEASOCKS_DEFINE_RESPONSECODE(303, SeeOther, "See Other")
SEASOCKS_DEFINE_RESPONSECODE(304, NotModified, "Not Modified")
SEASOCKS_DEFINE_RESPONSECODE(305, UseProxy, "Use Proxy")
SEASOCKS_DEFINE_RESPONSECODE(306, SwitchProxy, "Switch Proxy")
SEASOCKS_DEFINE_RESPONSECODE(307, TemporaryRedirect, "Temporary Redirect")
SEASOCKS_DEFINE_RESPONSECODE(308, ResumeIncomplete, "Resume Incomplete")

SEASOCKS_DEFINE_RESPONSECODE(400, BadRequest, "Bad Request")
SEASOCKS_DEFINE_RESPONSECODE(401, Unauthorized, "Unauthorized")
SEASOCKS_DEFINE_RESPONSECODE(402, PaymentRequired, "Payment Required")
SEASOCKS_DEFINE_RESPONSECODE(403, Forbidden, "Forbidden")
SEASOCKS_DEFINE_RESPONSECODE(404, NotFound, "Not Found")
SEASOCKS_DEFINE_RESPONSECODE(405, MethodNotAllowed, "Method Not Allowed")
// more here...
SEASOCKS_DEFINE_RESPONSECODE(426, UpgradeRequired, "Upgrade Required")
// more here...

SEASOCKS_DEFINE_RESPONSECODE(500, InternalServerError, "Internal Server Error")
SEASOCKS_DEFINE_RESPONSECODE(501, NotImplemented, "Not Implemented")

#endif
