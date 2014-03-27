SeaSocks - A tiny embeddable C++ HTTP and WebSocket server
==========================================================

[![Build Status](https://travis-ci.org/mattgodbolt/seasocks.svg?branch=master)](https://travis-ci.org/mattgodbolt/seasocks)

Features
--------
*   Simple C++ API
*   Serves static content from disk
*   API for building WebSocket servers
*   Simple command line binary for quick serving of static files only
*   Supports newer Hybi-10 and Hybi-16 WebSockets as well as the older Hixie style.

Stuff it doesn't do
-------------------
It's not nearly as configurable as Apache, Lighttpd, Nginx, Jetty, etc.
It provides only very limited support for custom content generation (e.g. Servlets).
It has been optimized for WebSocket based control.

Getting started
---------------
Check out the [tutorial](https://github.com/mattgodbolt/seasocks/wiki/SeaSocks-quick-tutorial) on the wiki.

See [src/app/c/ws_test.cpp](https://github.com/mattgodbolt/seasocks/blob/master/src/app/c/ws_test.cpp) for an example.
