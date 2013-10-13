m4_include([scripts/c++11.m4])

AC_PREREQ(2.65)
AC_INIT([SeaSocks], [0.1], [matt@godbolt.org], [], [https://github.com/mattgodbolt/seasocks])

AC_LANG(C++)
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP

AC_CONFIG_SRCDIR([src/main/c/seasocks/Server.h])
#AC_CONFIG_HEADERS([config.h])

CXXFLAGS="$CXXFLAGS -pthread"

AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h limits.h netinet/in.h stddef.h stdlib.h string.h strings.h sys/ioctl.h sys/socket.h unistd.h])

AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_PID_T
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])
AC_CHECK_HEADERS([tclap/CmdLine.h])

AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_STRERROR_R
AC_CHECK_FUNCS([dup2 gethostname memset rmdir socket sqrt strcasecmp strchr strdup strerror])

AX_CXX_COMPILE_STDCXX_11([noext])

SAVED_LDFLAGS=$LDFLAGS
LDFLAGS="$LDFLAGS -lgmock -lgtest"
AC_LINK_IFELSE([AC_LANG_PROGRAM([#include <gmock/gmock.h>],
               [testing::Cardinality dummy])],
               [TEST_LIBS="$TEST_LIBS -lgmock -lgtest"] [HAVE_GMOCK=1],
               [AC_MSG_WARN([libgmock is not installed.])])
LDFLAGS=$SAVED_LDFLAGS

AC_SUBST(HAVE_GMOCK,0)
AC_SUBST(TEST_LIBS)
AC_OUTPUT([Makefile])
