DEFS = @DEFS@ @sound_flag@ @dl_flag@ @inst_flag@

bin_PROGRAMS = lgeneral

AM_CPPFLAGS = -I$(top_srcdir)/util $(INTLINCLUDES)
lgeneral_SOURCES = main.cpp \
config.cpp date.cpp \
file.cpp map.cpp nation.cpp \
player.cpp scenario.cpp lg-sdl.cpp misc.cpp unit.cpp \
audio.cpp parser.cpp list.cpp unit_lib.cpp \
terrain.cpp image.cpp slot.cpp gui.cpp purchase_dlg.cpp \
event.cpp windows.cpp action.cpp campaign.cpp \
strat_map.cpp ai.cpp engine.cpp ai_group.cpp ai_tools.cpp \
lgeneral.6

AM_CFLAGS = @SDL_CFLAGS@
lgeneral_LDFLAGS = @export_flag@
lgeneral_LDADD   = @mixer_flag@ @dl_lib_flag@ $(top_builddir)/util/libutil.a $(INTLLIBS) @SDL_LIBS@
man_MANS = lgeneral.6

EXTRA_DIST = lgeneral.h \
config.h date.h engine.h \
file.h map.h nation.h player.h \
player.h scenario.h lg-sdl.h misc.h unit.h unit.h \
audio.h parser.h list.h unit_lib.h \
terrain.h image.h slot.h gui.h purchase_dlg.h \
event.h windows.h action.h campaign.h \
strat_map.h ai.h ai_group.h ai_tools.h

SUBDIRS = gfx themes campaigns maps nations sounds music units \
scenarios ai_modules terrain

check_PROGRAMS = testfile

testfile_CFLAGS = -DTESTFILE @SDL_CFLAGS@
testfile_SOURCES = file.cpp list.cpp misc.cpp
testfile_LDADD = $(top_builddir)/util/libutil.a

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(inst_dir)

uninstall-local:
	rm -rf $(DESTDIR)$(inst_dir)
