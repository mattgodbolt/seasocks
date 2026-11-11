Seasocks - A tiny embeddable C++ HTTP and WebSocket server
====================================================================

[![CI](https://github.com/mattgodbolt/seasocks/workflows/ci/badge.svg)](https://github.com/mattgodbolt/seasocks/actions)
[![codecov](https://codecov.io/gh/mattgodbolt/seasocks/branch/master/graph/badge.svg)](https://codecov.io/gh/mattgodbolt/seasocks)
[![GitHub release](https://img.shields.io/github/release/mattgodbolt/seasocks.svg)](https://github.com/mattgodbolt/seasocks/releases)
[![License](https://img.shields.io/badge/license-BSD-yellow.svg)](LICENSE)
[![ConanCenter package](https://repology.org/badge/version-for-repo/conancenter/seasocks.svg)](https://conan.io/center/seasocks)

Features
--------
* Simple C++ API
* Serves static content from disk
* API for building WebSocket servers
* Simple command line binary for quick serving of static files only
* Supports newer Hybi-10 and Hybi-16 WebSockets as well as the older Hixie style.
* Supports Linux and Windows

Stuff it doesn't do
-------------------
* It's not nearly as configurable as Apache, Lighttpd, Nginx, Jetty, etc.
* It provides only limited support for custom content generation (e.g. Servlets).
* It has been designed for WebSocket based control.
* It probably won't work on other systems (patches to support other systems would be welcome, but not a priority for us)

Getting started
---------------
Check out the [tutorial](https://github.com/mattgodbolt/seasocks/wiki/Seasocks-quick-tutorial) on the wiki.

See [src/app/c/ws_test.cpp](https://github.com/mattgodbolt/seasocks/blob/master/src/app/c/ws_test.cpp) for an example.


Using Seasocks in your CMake project
------------------------------------
Seasocks needs a C++17 compiler. Once Seasocks is installed (`cmake --install`), find it and link against the `Seasocks::seasocks` target:

```cmake
find_package(Seasocks REQUIRED)
add_executable(my_server serve.cpp)
target_compile_features(my_server PRIVATE cxx_std_17)
target_link_libraries(my_server PRIVATE Seasocks::seasocks)
```

If you installed to a non-standard prefix, add it to `CMAKE_PREFIX_PATH`.

Alternatively, build Seasocks as part of your project, either with `add_subdirectory` on a copy of the source (for example a git submodule in `thirdparty/seasocks`) or with `FetchContent`. The link line is the same:

```cmake
set(UNITTESTS OFF CACHE INTERNAL "")
set(SEASOCKS_EXAMPLE_APP OFF CACHE INTERNAL "")
add_subdirectory(thirdparty/seasocks)
target_compile_features(my_server PRIVATE cxx_std_17)
target_link_libraries(my_server PRIVATE Seasocks::seasocks)
```

Seasocks is built as a static library unless you set `SEASOCKS_SHARED=ON` (or `BUILD_SHARED_LIBS=ON`). Deflate support is on by default and requires zlib; configure with `-DDEFLATE_SUPPORT=OFF` to build without it. Projects linking a static, deflate-enabled install need zlib to be findable too.
