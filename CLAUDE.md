# CLAUDE.md

Seasocks: small embeddable C++17 HTTP/WebSocket server library (CMake). Linux and Windows (MSVC) only;
CMake hard-fails on Apple.

## Build and test
```sh
cmake -S . -B build -GNinja -DCMAKE_BUILD_TYPE=Release   # needs Python (embed step) and zlib
cmake --build build
ctest --test-dir build --output-on-failure               # one ctest entry: AllTests
build/src/test/c/AllTests "[CrackedUriTests]"            # one tag; or pass a test-case name
ctest --test-dir build -T memcheck                       # valgrind, suppressions in src/test/suppressions.txt
```
- Options: `-DDEFLATE_SUPPORT=OFF` (no zlib; CI builds this on Linux and MSVC), `-DUNITTESTS=OFF`,
  `-DSEASOCKS_EXAMPLE_APP=OFF`, `-DSEASOCKS_SHARED=ON`, `-DCOVERAGE=ON`.
- Catch2 v3 is fetched with FetchContent at configure time (needs network on first configure).
- Warnings are errors: `-Wall -Werror -Wextra -pedantic -pedantic-errors` (GCC/Clang), `/W4 /WX` (MSVC).
- CI (`.github/workflows/ci.yml`): GCC 12-16 and Clang 18-22 (Clang with `-stdlib=libc++`), Release +
  coverage, then ctest + valgrind memcheck; plus Windows MSVC Debug with deflate off; plus CodeQL.

## Layout
- `src/main/c/` library: public headers in `seasocks/` (installed), private ones in `internal/`,
  `seasocks/win32/` Windows shims (wepoll etc.), vendored `md5/`, `sha1/`.
- `src/main/web/` files compiled into the library by `scripts/gen_embedded.py` -> `Embedded.cpp`.
- `src/app/c/` example apps (`ws_test`, `async_test`, `serve`...); `src/*_web/` are their web assets.
- `src/test/c/` Catch2 tests, all linked into the single `AllTests` binary.
- Every source file is listed explicitly in its directory's `CMakeLists.txt` (no globbing): add new
  library/test files there. `HttpCompressionTests.cpp` and `ZlibContext.cpp` are only built with deflate on.

## Gotchas
- Embedded content: a new file in `src/main/web/` must be added to `EMBEDDED_FILES` in its CMakeLists.
  The generate step only depends on the script, so editing an embedded file does not rebuild it:
  delete `build/src/main/web/Embedded.cpp` (or reconfigure) after changing one.
- Threading: the server is single-threaded. `loop()`/`poll()` and everything on `Connection`/`WebSocket`
  (e.g. `send`, `close`) and `ResponseWriter` must run on the seasocks thread (`Server::checkThread()`
  throws otherwise). From other threads only `Server::terminate()` and `Server::execute(...)` are safe;
  use `execute` to marshal work onto the loop (see `src/app/c/async_test.cpp`).
- Portability: platform code is guarded with `#ifdef _WIN32` (Winsock/wepoll vs POSIX/epoll);
  `ws_test_poll` is not built on Windows. Keep both paths compiling.
- Exceptions are used (e.g. `std::runtime_error` from `CrackedUri`, zlib, `checkThread`).

## Style
- `.clang-format` (4-space indent, no column limit, `PointerAlignment: Left`, includes not sorted).
  Not enforced by CI. Run `clang-format -i` on files you touch; avoid the `clang-format` build target,
  which also rewrites vendored md5/sha1/win32 files that are not clean.
- Most files start with the BSD license header from `LICENSE`; copy it into new files.

## Commits / PRs
- PRs are squash-merged; subject is short imperative, suffixed with `(#NNN)` on merge. Body explains
  the why and lists tests. No CHANGELOG; version lives in `project(Seasocks VERSION ...)` in CMakeLists.txt.
