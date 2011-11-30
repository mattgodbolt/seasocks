// Not a normal header file - no header guards on purpose.
// Do not directly include! Use ResponseCode.h instead

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

SEASOCKS_DEFINE_RESPONSECODE(500, InternalServerError, "Internal Server Error")
SEASOCKS_DEFINE_RESPONSECODE(501, NotImplemented, "Not Implemented")
