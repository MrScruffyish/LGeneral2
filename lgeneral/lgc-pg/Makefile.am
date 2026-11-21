bin_PROGRAMS = lgc-pg shptool
lgc_pg_SOURCES = main.cpp units.cpp shp.cpp nations.cpp terrain.cpp maps.cpp scenarios.cpp \
list.cpp misc.cpp parser.cpp lgc-pg.1
lgc_pg_CXXFLAGS = @SDL_CFLAGS@
LIBS = $(top_builddir)/util/libutil.a @SDL_LIBS@
DEFS = @DEFS@ @inst_flag@
AM_CPPFLAGS = -I$(top_srcdir)

shptool_SOURCES = shptool.cpp misc.cpp shp.cpp
shptool_CXXFLAGS = -DSHPTOOL @SDL_CFLAGS@

man_MANS = lgc-pg.1

EXTRA_DIST = units.h shp.h nations.h terrain.h maps.h scenarios.h \
list.h misc.h parser.h shptool.cpp

SUBDIRS = convdata

