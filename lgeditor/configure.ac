#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(lgeditor, 0.3)
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB(m, main,, AC_MSG_ERROR(lib math is needed))
AC_CHECK_LIB(SDL2, main,, AC_MSG_ERROR(lib SDL2 is needed))
AC_CHECK_LIB(SDL2_ttf, main,, AC_MSG_ERROR(lib SDL2 ttf is needed))
AC_CHECK_LIB(SDL2_image, main,, AC_MSG_ERROR(lib SDL2 image is needed))

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([memset strchr strdup strrchr])

# Flags
datdir=$datadir/$PACKAGE
AC_ARG_ENABLE( install,
[  --disable-install       No installation. Start from source directory.], 
datdir="../data/")

dnl The editor loads graphics and maps from the game's data directory, so it
dnl must be told where that directory is. Automatic discovery used to scan all
dnl of /usr, which was slow and unreliable; require an explicit path instead.
AC_ARG_WITH( [lgendir],
  [AS_HELP_STRING([--with-lgendir=DIR],
                  [directory containing the LGeneral graphics and maps (required)])],
  [
    if test -z "$withval" || test "$withval" = yes || test "$withval" = no; then
      AC_MSG_ERROR([--with-lgendir requires a directory])
    fi
    if test ! -d "$withval"; then
      AC_MSG_ERROR([--with-lgendir directory does not exist: $withval])
    fi
    lgendir=$withval
  ],
  [AC_MSG_ERROR([please specify the LGeneral data directory with --with-lgendir=DIR])])
AC_MSG_RESULT([LGeneral location: $lgendir])

CXXFLAGS="$CXXFLAGS $(sdl2-config --cflags) -Wall"
CXXFLAGS="$CXXFLAGS -DDATADIR=\\\"$datdir\\\" -DLGENDIR=\\\"$lgendir\\\""
LIBS="$LIBS $(sdl2-config --libs)"

# Makefiles
AC_CONFIG_FILES([Makefile src/Makefile data/Makefile])
AC_OUTPUT

