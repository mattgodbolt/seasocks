SeaSocks - A tiny embeddable C++ HTTP and WebSocket server
==========================================================

Features
--------
*   Simple C++ API
*   Serves static content from disk
*   API for building WebSocket servers
*   Integration with [DRW Single Sign On](http://go/sso)
*   Simple command line binary for quick serving of static files only

Stuff it doesn't do
-------------------
It's not nearly as configurable as Apache, Lighttpd, Nginx, Jetty, etc.
It provides no support for custom content generation (e.g. Servlets).
It has been optimized for WebSocket based control.

Getting started
---------------
See src/app/c/ws_test.cpp for an example.

