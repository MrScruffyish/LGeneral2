# Top-level build for LGeneral2.
#
# Builds the game (lgeneral/) and the map editor (lgeditor/) in .build/, then
# converts the Panzer General data set into the game's data directory there.
#
#   make                                   # build everything
#   PG_DATA=/path/to/pg-data.tar.gz make    # tarball or unpacked directory
#   make run                               # start the game from its data dir
#   make clean                             # remove build output

SHELL := /bin/sh

JOBS ?= $(shell nproc)

# TRACE=1 builds a separate copy with deployment/turn logging enabled.
# Output goes to /tmp/human.log (override with LG_TRACE_FILE).
TRACE ?= 0
TRACE_SUFFIX := $(if $(filter 1,$(TRACE)),-trace,)
TRACE_CFLAGS := $(if $(filter 1,$(TRACE)),-DLG_TRACE -O0 -g,)

# Path to pg-data.tar.gz or an unpacked pg-data directory, supplied through
# the environment (e.g. export PG_DATA=~/PG_DATA/pg-data.tar.gz).
PG_DATA ?=

LGENERAL := lgeneral
LGEDITOR := lgeditor

BUILD_ROOT := $(CURDIR)/.build
LGENERAL_BUILD := $(BUILD_ROOT)/lgeneral$(TRACE_SUFFIX)
LGEDITOR_BUILD := $(BUILD_ROOT)/lgeditor$(TRACE_SUFFIX)
GAME_DATA := $(LGENERAL_BUILD)/src
EXTRACTED_PG_DATA := $(BUILD_ROOT)/pg-data

# Extra options passed to the sub-projects' configure scripts.
LGENERAL_CONFIGURE_ARGS ?=
LGEDITOR_CONFIGURE_ARGS ?=

# lgc-pg only needs SDL's image routines; a dummy driver keeps data
# conversion usable on headless build machines.
SDL_VIDEODRIVER ?= dummy

.PHONY: all lgeneral lgeditor data run clean distclean

all: lgeneral lgeditor data

$(LGENERAL_BUILD)/.synced: $(shell find $(LGENERAL) -type f) Makefile
	rm -rf $(LGENERAL_BUILD)
	mkdir -p $(LGENERAL_BUILD)
	cp -a $(LGENERAL)/. $(LGENERAL_BUILD)/
	touch $@

$(LGEDITOR_BUILD)/.synced: $(shell find $(LGEDITOR) -type f) Makefile
	rm -rf $(LGEDITOR_BUILD)
	mkdir -p $(LGEDITOR_BUILD)
	cp -a $(LGEDITOR)/. $(LGEDITOR_BUILD)/
	touch $@

# Translations stay disabled for now; the build uses system gettext and
# bootstraps the autotools files in the isolated build copy.
lgeneral: $(LGENERAL_BUILD)/.synced
	@echo "==> Building $(LGENERAL)"
	@if [ "$(TRACE)" = 1 ]; then \
		cp debug/lgtrace.h $(LGENERAL_BUILD)/src/; \
		python3 debug/instrument.py $(LGENERAL_BUILD); \
	fi
	cd $(LGENERAL_BUILD) && CFLAGS="$(CFLAGS) $(TRACE_CFLAGS)" autoreconf -fi && \
		CFLAGS="$(CFLAGS) $(TRACE_CFLAGS)" ./configure --disable-install \
		--disable-nls $(LGENERAL_CONFIGURE_ARGS)
	$(MAKE) -C $(LGENERAL_BUILD) -j$(JOBS)
	@printf '%s\n' '@' 'grid=0' 'tran=1' 'bar=1' 'width=800' 'height=600' \
		'fullscreen=0' 'anim_speed=1' 'supply=1' 'weather=1' \
		'fog_of_war=1' 'cpu_turn=1' 'deploy=1' 'purchase=1' \
		'ai_debug=0' 'sound_on=1' 'sound_volume=96' 'music_on=1' \
		'music_volume=96' > $(GAME_DATA)/lgeneral.conf

lgeditor: $(LGEDITOR_BUILD)/.synced
	@echo "==> Building $(LGEDITOR)"
	cd $(LGEDITOR_BUILD) && autoreconf -fi && ./configure --disable-install \
		--with-lgendir=$(GAME_DATA) $(LGEDITOR_CONFIGURE_ARGS)
	$(MAKE) -C $(LGEDITOR_BUILD) -j$(JOBS)

data: lgeneral
	@echo "==> Converting Panzer General data"
	@set -e; \
	if [ -z "$(strip $(PG_DATA))" ]; then \
		echo "PG_DATA is empty; skipping data conversion" >&2; \
	elif [ -d "$(PG_DATA)" ]; then \
		source_dir=$$(cd "$(PG_DATA)" && pwd); \
	elif [ -f "$(PG_DATA)" ]; then \
		source_dir="$(EXTRACTED_PG_DATA)"; \
		rm -rf "$$source_dir"; \
		mkdir -p "$$source_dir"; \
		tar -xzf "$(PG_DATA)" -C "$$source_dir" --strip-components=1; \
	else \
		echo "PG_DATA not found: $(PG_DATA)" >&2; \
		exit 1; \
	fi; \
	if [ -n "$$source_dir" ]; then \
		rm -rf $(GAME_DATA)/convdata; \
		cp -a $(LGENERAL_BUILD)/lgc-pg/convdata $(GAME_DATA)/convdata; \
		cd $(GAME_DATA) && \
			SDL_VIDEODRIVER=$(SDL_VIDEODRIVER) ../lgc-pg/lgc-pg \
				-s "$$source_dir" -d .; \
	fi

# The game looks up data (and its config file) relative to the working
# directory, so it must be started from the data directory.
run:
	cd $(GAME_DATA) && ./lgeneral

clean:
	rm -rf $(BUILD_ROOT)

distclean: clean
