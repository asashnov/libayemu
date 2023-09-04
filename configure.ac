# Process this file with autoconf to produce a configure script.
AC_INIT([libayemu], [1.1.0])
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE

# Options
# AC_ARG_ENABLE

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(limits.h)
AC_CHECK_HEADERS(stddef.h)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset])

# wanted by: test/test.c:10
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([sys/ioctl.h])

AC_MSG_CHECKING(--enable-apps argument)
AC_ARG_ENABLE(apps,
    [  --enable-apps           build and install apps.],
    [enable_apps=$enableval],
    [enable_apps="yes"])
AM_CONDITIONAL([ENABLE_APPS], [test "x$enable_apps" = "xyes"])
AC_MSG_RESULT($enable_apps)

AC_MSG_CHECKING(--enable-tests argument)
AC_ARG_ENABLE(tests,
    [  --enable-tests          build and install tests.],
    [enable_tests=$enableval],
    [enable_tests="yes"])
AC_MSG_RESULT($enable_tests)
AM_CONDITIONAL([ENABLE_TESTS], [test "x$enable_tests" = "xyes"])

AC_OUTPUT(Makefile include/Makefile src/Makefile test/Makefile apps/Makefile apps/playvtx/Makefile)
