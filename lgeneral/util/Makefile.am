noinst_LIBRARIES = libutil.a
libutil_a_SOURCES = localize.cpp localize.h hash-string.h \
	hashtable.cpp hashtable.h hashtable_private.h \
	hashtable_itr.cpp hashtable_itr.h \
	hashtable_utility.cpp hashtable_utility.h \
	paths.cpp paths.h portability.cpp $(PATHS_PLATFORM)

if compile_paths_linux
  PATHS_PLATFORM = paths-linux.cpp
endif

AM_CPPFLAGS = $(INTLINCLUDES) -I$(top_srcdir)
DEFS = -DHAVE_CONFIG_H -DDATADIR=\"@datadir@\" -DPREFIX=\"@prefix@\"

