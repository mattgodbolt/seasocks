m4_include([scripts/c++11.m4])

AC_PREREQ(2.65)
AC_INIT([SeaSocks], [0.1], [matt@godbolt.org], [], [https://github.com/mattgodbolt/seasocks])
AC_CONFIG_SRCDIR([src/main/c/seasocks/Server.h])

AC_ARG_WITH([gmock],
            [AS_HELP_STRING([--with-gmock],
                            [Build tests too, with supplied gmock directory.  SeaSocks will
                             build gmock and its embedded gtest automatically])],
                             [],
                             [AC_MSG_WARN([dnl
Building without tests enabled. Use --with-gmock=PATH_TO_GMOCK])])

AS_IF([test "x${with_gmock}" != "xno" -a "x${with_gmock}" != "x"],
      [
       AS_IF([test -e "${with_gmock}/include/gmock/gmock.h"],
             [GMOCK_DIR="${with_gmock}"],
             [AC_MSG_ERROR([dnl
              Unable to locate gmock source at '${with_gmock}'.])])
       ])
AC_SUBST(GMOCK_DIR)

dnl Basic setup
AC_LANG(C++)
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX_11([noext])

dnl Basic headers and features
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_PID_T
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_STRERROR_R
AC_CHECK_TYPES([ptrdiff_t])

dnl pthreads; better if we detect this somehow
CXXFLAGS="$CXXFLAGS -pthread"

dnl System headers and functions used
AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h limits.h netinet/in.h stddef.h stdlib.h string.h strings.h sys/ioctl.h sys/socket.h unistd.h])
AC_CHECK_FUNCS([dup2 eventfd syscall gethostname memset rmdir socket sqrt strcasecmp strchr strdup strerror])

dnl Non-standard headers used
AC_CHECK_HEADERS([tclap/CmdLine.h])

AC_OUTPUT([Makefile])
