dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(lgeneral,1.4.4)
AC_CANONICAL_TARGET
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE
AC_USE_SYSTEM_EXTENSIONS

dnl Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB

dnl various CFLAGS
CFLAGS="$CFLAGS -Wall -std=gnu11"

dnl Checks for libraries.
AC_CHECK_LIB(m, main,, AC_MSG_ERROR(maths library is needed))
dnl AC_CHECK_LIB(dl, main,, AC_CHECK_LIB(c, dlopen,, AC_MSG_ERROR(dl library is needed)))
dnl AC_CHECK_LIB(dl, main,, AC_MSG_ERROR(dl library is needed))

dnl Checks for functions.
AC_CHECK_FUNCS([strcasestr setenv])

dnl check for mkdir accepting permissions
AC_MSG_CHECKING(if mkdir rejects permissions)
ac_mkdir_perm_broken=yes
AC_COMPILE_IFELSE(
  [AC_LANG_PROGRAM([[#include <unistd.h>
#include <sys/stat.h>]], [[mkdir("test", 0777)]])],
  [ac_mkdir_perm_broken=no])
if test x$ac_mkdir_perm_broken = xyes ; then
  AC_DEFINE_UNQUOTED(HAVE_BROKEN_MKDIR, 1, [Defined if mkdir rejects a second permissions parameter.])
fi
AC_MSG_RESULT($ac_mkdir_perm_broken)

dnl L10n support
ALL_LINGUAS="en de"

AM_GNU_GETTEXT_VERSION([0.21])
AM_GNU_GETTEXT([external])

dnl win32 stuff
dnl USE_NLS=no
dnl USE_INCLUDED_LIBINTL=no
dnl INTLLIBS=-lintl
dnl AC_SUBST(USE_NLS)
dnl AC_SUBST(USE_INCLUDED_LIBINTL)
dnl AC_SUBST(INTLLIBS)

XGETTEXTFLAGS=
MSGFMTFLAGS=-v
MSGMERGEFLAGS=
MSGCONVFLAGS=--to-code=iso-8859-1
INTLINCLUDES=

dnl check for msgconv
AC_PATH_PROG(MSGCONV, msgconv, cat)
dnl reset flags if msgconv is "dummy"
test "x$MSGCONV" = xcat && MSGCONVFLAGS=

dnl FIXME: This does no logging whatsoever if anything goes wrong
if test x$USE_NLS = xyes -a x$XGETTEXT != "x:" ; then
  AC_MSG_CHECKING(whether xgettext needs --from-code)
  ac_xgettext_needs_from_code=error
  ac_xgettext_from_code=--from-code=iso-8859-1
  cat <<EOF >ac_tmp.c
int main() { _("�������"); }
EOF
  $XGETTEXT -k_ $ac_xgettext_from_code ac_tmp.c >/dev/null 2>&1 && ac_xgettext_needs_from_code=yes
  $XGETTEXT -k_ ac_tmp.c >/dev/null 2>&1 && ac_xgettext_needs_from_code=no
  AC_MSG_RESULT($ac_xgettext_needs_from_code)
  if test $ac_xgettext_needs_from_code = yes ; then
    XGETTEXTFLAGS="$XGETTEXTFLAGS $ac_xgettext_from_code"
  elif test $ac_xgettext_needs_from_code = error ; then
    AC_MSG_ERROR(Both invocations of $XGETTEXT failed!)
  fi
  rm -f ac_tmp.c
fi

dnl libintl.h is supplied by the system gettext (external mode).
INTLINCLUDES=

dnl The po/ Makefiles are generated from Makefile.in.in through
dnl AC_CONFIG_FILES below, so autoconf needs these values at configure time.
dnl External gettext leaves them for config.status (which runs too late for
dnl us), so provide them here. Modern gettext compiles .po straight to .gmo;
dnl CATOBJEXT is empty in external mode, so spell the suffix out explicitly.
if test "x$POFILES" = x ; then
  for l in $ALL_LINGUAS ; do
    POFILES="$POFILES$l.po "
  done
  AC_SUBST(POFILES)
fi
if test "x$GMOFILES" = x ; then
  for l in $ALL_LINGUAS ; do
    GMOFILES="$GMOFILES$l.gmo "
  done
  AC_SUBST(GMOFILES)
fi
if test "x$CATALOGS" = x ; then
  AC_MSG_CHECKING([for catalogs to be installed])
  for l in $ALL_LINGUAS ; do
    CATALOGS="$CATALOGS$l.gmo "
  done
  AC_MSG_RESULT([$ALL_LINGUAS])
  AC_SUBST(CATALOGS)
fi

AC_SUBST(XGETTEXTFLAGS)
AC_SUBST(MSGFMTFLAGS)
AC_SUBST(MSGMERGEFLAGS)
AC_SUBST(MSGCONVFLAGS)
AC_SUBST(INTLINCLUDES)

dnl find out whether we can determine the path of the running executable
ac_paths_determ=generic
  ac_can_determ_paths=no
  AC_DEFINE_UNQUOTED(PATHS_GENERIC, 1, [Defined if detecting executable's path name is not supported at runtime.])
AC_MSG_RESULT($ac_can_determ_paths)
AC_MSG_RESULT(executable's path determination strategy... $ac_paths_determ)
AM_CONDITIONAL(compile_paths_linux, test x$ac_paths_determ = xlinux)

dnl SDL3 is used by the game, the converter and the editor toolchain.
PKG_CHECK_MODULES([SDL3], [sdl3], [],
                  [AC_MSG_ERROR([lib SDL3 is needed])])

dnl check sound
sound_flag=""
mixer_flag=""
AC_ARG_ENABLE( sound,
[  --disable-sound         Disables sound], [enable_sound=no], [enable_sound=yes])

dnl check if SDL3_mixer is installed
if test "x$enable_sound" = xyes ; then
  PKG_CHECK_MODULES([SDL3_MIXER], [sdl3-mixer >= 3.2.0],
                    [sound_flag="-DWITH_SOUND"; mixer_flag="$SDL3_MIXER_LIBS"],
                    [AC_MSG_RESULT([SDL3_mixer not found: audio disabled])])
fi
AC_SUBST(sound_flag)
AC_SUBST(mixer_flag)

dnl check use of dynamic libraries
dnl SINCE NEVER USED TEMPORARILY DISABLED MS
dnl dl_lib_flag="-ldl"
dnl dl_flag="-DUSE_DL"
dnl export_flag="-Wl,-export-dynamic"
dl_lib_flag=""
dl_flag=""
export_flag=""
AC_ARG_ENABLE( dl,
[  --disable-dl         Disable use of dynamic AI modules], dl_flag=""; export_flag=""; dl_lib_flag="";)
AC_SUBST(dl_flag)
AC_SUBST(export_flag)
AC_SUBST(dl_lib_flag)

dnl installation path
inst_dir=$datadir/lgeneral
inst_flag="-DINSTALLDIR=\\\"$inst_dir\\\""

dis_flag=""
AC_ARG_ENABLE( install,
[  --disable-install       No installation. Played from the source directory.], 
inst_dir=.
inst_flag=)

AC_SUBST(inst_dir)
AC_SUBST(inst_flag)

dnl Where gettext looks for the compiled catalogs. With --disable-install the
dnl game runs from its data directory (like the rest of its data), so use a
dnl path relative to the working directory in that case.
if test x$inst_dir = x. ; then
  AC_DEFINE_UNQUOTED([LOCALEDIR], ["share/locale"],
    [Directory holding gettext catalogs, relative to the cwd in uninstalled builds])
else
  AC_DEFINE_UNQUOTED([LOCALEDIR], ["$prefix/share/locale"],
    [Directory holding gettext catalogs])
fi

AC_CONFIG_FILES([
 Makefile
 lged/Makefile
 lgc-pg/Makefile
 lgc-pg/convdata/Makefile
 src/Makefile
 src/nations/Makefile
 src/scenarios/Makefile
 src/units/Makefile
 src/sounds/Makefile
 src/music/Makefile
 src/terrain/Makefile
 src/maps/Makefile
 src/gfx/Makefile
 src/gfx/flags/Makefile
 src/gfx/terrain/Makefile
 src/gfx/units/Makefile
 src/ai_modules/Makefile
 src/campaigns/Makefile
 src/themes/Makefile
 src/themes/default/Makefile
 util/Makefile
 po/Makefile
 po/lgeneral/Makefile.in
 po/pg/Makefile.in
 tools/Makefile
 tools/ltrextract/Makefile
])
AC_OUTPUT

