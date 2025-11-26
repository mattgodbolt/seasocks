#pragma once

#ifdef _WIN32
#define NOMINMAX 1
#include <winsock2.h>

#ifndef TCP_KEEPALIVE
#define TCP_KEEPALIVE 3
#define TCP_KEEPIDLE TCP_KEEPALIVE
#define TCP_KEEPCNT 16
#define TCP_KEEPINTVL 17
#endif
#endif
