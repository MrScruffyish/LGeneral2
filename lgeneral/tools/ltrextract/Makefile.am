LGENERAL_PATH = $(top_srcdir)/src
AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = ltrextract

AM_CPPFLAGS = -DLTREXTRACT -I$(LGENERAL_PATH) -I$(top_srcdir) -I$(top_srcdir)/util $(INTLINCLUDES)
ltrextract_LDADD = $(top_builddir)/util/libutil.a $(INTLLIBS)
ltrextract_SOURCES = ltrextract.cpp util.cpp util.h \
	terrain.cpp terrain.h map.cpp map.h nation.cpp nation.h \
	unit_lib.cpp unit_lib.h scenario.cpp scenario.h campaign.cpp campaign.h \
	$(LGENERAL_PATH)/parser.c \
	$(LGENERAL_PATH)/list.c

