bin_PROGRAMS = lgc-pg shptool
lgc_pg_SOURCES = main.cpp units.cpp shp.cpp nations.cpp terrain.cpp maps.cpp scenarios.cpp \
list.cpp misc.cpp parser.cpp lgc-pg.1
lgc_pg_CXXFLAGS = @SDL3_CFLAGS@
lgc_pg_LDADD = $(top_builddir)/util/libutil.a @SDL3_LIBS@
DEFS = @DEFS@ @inst_flag@
AM_CPPFLAGS = -I$(top_srcdir)

shptool_SOURCES = shptool.cpp misc.cpp shp.cpp
shptool_CXXFLAGS = -DSHPTOOL @SDL3_CFLAGS@
shptool_LDADD = $(top_builddir)/util/libutil.a @SDL3_LIBS@

man_MANS = lgc-pg.1

EXTRA_DIST = sdl3_compat.h units.h shp.h nations.h terrain.h maps.h scenarios.h \
list.h misc.h parser.h shptool.cpp

SUBDIRS = convdata

