AC_INIT([cm], [0.1], [issues@commonmark.org])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

AC_PROG_CC_C99
AM_PROG_CC_C_O

AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

AC_CHECK_HEADERS([inttypes.h])
AC_CHECK_HEADERS([limits.h])
AC_CHECK_HEADERS([stddef.h])
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([string.h])
AC_CHECK_HEADER_STDBOOL
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([strerror])

AC_C_INLINE
AC_FUNC_MALLOC

AC_CHECK_PROG(PERL, perl, perl, missing)

if test "$PERL" == missing; then
    AC_MSG_ERROR([perl cannot be found in PATH, please install it])
fi

AC_CHECK_PROG(HAVE_PANDOC, pandoc, pandoc, missing)

if test "$PANDOC" == missing; then
    AC_MSG_ERROR([pandoc cannot be found in PATH, please install it])
fi

AC_CHECK_PROG(RE2C, re2c, re2c, missing)

if test "$RE2C" == missing; then
    AC_MSG_ERROR([re2c cannot be found in PATH, please install it])
fi

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
