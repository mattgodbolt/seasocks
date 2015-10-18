
find_package(PkgConfig)
pkg_check_modules(PKG_gmock QUIET gmock)

set(gmock_DEFINITIONS ${PKG_gmock_CFLAGS_OTHER})

find_path(gmock_INCLUDE_DIR "gmock/gmock.h"
                            HINTS ${PKG_gmock_INCLUDE_DIRS}
                            PATH_SUFFIXES "gmock"
                            )
find_library(gmock_LIBRARY NAMES gmock
                            HINTS ${PKG_gmock_LIBDIR} ${PKG_gmock_LIBRARY_DIRS}
                            )

set(gmock_LIBRARIES ${gmock_LIBRARY})
set(gmock_INCLUDE_DIRS ${gmock_INCLUDE_DIR})


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(gmock DEFAULT_MSG gmock_LIBRARY gmock_INCLUDE_DIRS)

mark_as_advanced(gmock_INCLUDE_DIR gmock_LIBRARY)

