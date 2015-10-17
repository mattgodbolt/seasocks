
find_package(PkgConfig)
pkg_check_modules(PKG_gtest QUIET gtest)

set(gtest_DEFINITIONS ${PKG_gtest_CLFAGS_OTHER})

find_path(gtest_INCLUDE_DIR "gtest/gtest.h"
                            HINTS ${PKG_gtest_INCLUDE_DIRS}
                            PATH_SUFFIXES "gtest"
                            )
find_library(gtest_LIBRARY NAMES gtest
                            HINTS ${PKG_gtest_LIBDIR} ${PKG_gtest_LIBRARY_DIRS}
                            )

set(gtest_LIBRARIES ${gtest_LIBRARY})
set(gtest_INCLUDE_DIRS ${gtest_INCLUDE_DIR})


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(gmock DEFAULT_MSG gtest_LIBRARY gtest_INCLUDE_DIRS)

mark_as_advanced(gtest_INCLUDE_DIR gmock_LIBRARY)
 
