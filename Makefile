# Top-level build for LGeneral2.
#
# Builds the game (lgeneral/) and the map editor (lgeditor/) in .build/, then
# converts the Panzer General data set into the game's data directory there.
#
#   make                                   # build everything
#   PG_DATA=/path/to/pg-data.tar.gz make    # tarball or unpacked directory
#   SDL3_MIXER_PREFIX=/path/to/prefix make  # enable sound with SDL3_mixer >= 3.2
#   make run                               # start the game from its data dir
#   make smoke                             # clean builds + runtime launch test
#   make clean                             # remove build output

SHELL := /bin/sh

JOBS ?= $(shell nproc)

# TRACE=1 builds a separate copy with deployment/turn logging enabled.
# Output goes to /tmp/human.log (override with LG_TRACE_FILE).
TRACE ?= 0
TRACE_SUFFIX := $(if $(filter 1,$(TRACE)),-trace,)
TRACE_CMAKE := $(if $(filter 1,$(TRACE)),ON,OFF)

# Path to pg-data.tar.gz or an unpacked pg-data directory, supplied through
# the environment (e.g. export PG_DATA=~/PG_DATA/pg-data.tar.gz).
PG_DATA ?=

# Optional prefix containing sdl3-mixer >= 3.2. When set, both pkg-config and
# the runtime loader are pointed at it, so the resulting binary has sound
# without needing LD_LIBRARY_PATH at launch time.
SDL3_MIXER_PREFIX ?=
ifneq ($(strip $(SDL3_MIXER_PREFIX)),)
SDL3_MIXER_PREFIX := $(abspath $(SDL3_MIXER_PREFIX))
export SDL3_MIXER_PREFIX
SDL3_MIXER_PKG_CONFIG_PATH := $(SDL3_MIXER_PREFIX)/lib/pkgconfig:$(SDL3_MIXER_PREFIX)/lib64/pkgconfig
SDL3_MIXER_LIBDIR := $(shell PKG_CONFIG_PATH='$(SDL3_MIXER_PKG_CONFIG_PATH)$(if $(PKG_CONFIG_PATH),:$(PKG_CONFIG_PATH))' pkg-config --variable=libdir sdl3-mixer 2>/dev/null)
SDL3_MIXER_RPATH := $(if $(SDL3_MIXER_LIBDIR),$(SDL3_MIXER_LIBDIR),$(SDL3_MIXER_PREFIX)/lib:$(SDL3_MIXER_PREFIX)/lib64)
MIXER_CMAKE_ENV := PKG_CONFIG_PATH='$(SDL3_MIXER_PKG_CONFIG_PATH)$(if $(PKG_CONFIG_PATH),:$(PKG_CONFIG_PATH))'
MIXER_CMAKE_ARGS := -DCMAKE_PREFIX_PATH='$(SDL3_MIXER_PREFIX)' -DCMAKE_BUILD_RPATH='$(SDL3_MIXER_RPATH)'
else
MIXER_CMAKE_ENV :=
MIXER_CMAKE_ARGS :=
endif

LGENERAL := lgeneral
LGEDITOR := lgeditor

BUILD_ROOT := $(CURDIR)/.build
LGENERAL_BUILD := $(BUILD_ROOT)/lgeneral$(TRACE_SUFFIX)
LGEDITOR_BUILD := $(BUILD_ROOT)/lgeditor$(TRACE_SUFFIX)
GAME_DATA := $(LGENERAL_BUILD)/src
# Converted data is cached outside the disposable lgeneral build copy so a
# source-triggered rebuild does not silently remove scenarios and databases.
DATA_STAGE := $(BUILD_ROOT)/game-data
GAME_DATA_DIRS := ai_modules campaigns convdata gfx maps music nations scenarios sounds terrain themes units
EXTRACTED_PG_DATA := $(BUILD_ROOT)/pg-data

# Extra options passed to the sub-projects' CMake configuration steps.
LGENERAL_CMAKE_ARGS ?=
LGEDITOR_CMAKE_ARGS ?=

# Autotools used -g -O2 by default; keep the same baseline unless the caller
# supplies explicit flags.
ifeq ($(strip $(CFLAGS)),)
CMAKE_C_FLAGS := -g -O2 -Wall
else
CMAKE_C_FLAGS := $(strip $(CFLAGS) -Wall)
endif
ifeq ($(strip $(CXXFLAGS)),)
CMAKE_CXX_FLAGS := -g -O2 -Wall
else
CMAKE_CXX_FLAGS := $(strip $(CXXFLAGS) -Wall)
endif

# lgc-pg only needs SDL's image routines; a dummy driver keeps data
# conversion usable on headless build machines.

.PHONY: all lgeneral lgeditor data run smoke clean distclean

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

# The build uses system gettext and CMake inside the isolated build copy.
lgeneral: $(LGENERAL_BUILD)/.synced
	@echo "==> Building $(LGENERAL)"
	@if [ "$(TRACE)" = 1 ]; then \
		cp debug/lgtrace.h $(LGENERAL_BUILD)/src/; \
		python3 debug/instrument.py $(LGENERAL_BUILD); \
	fi
	$(MIXER_CMAKE_ENV) cmake -S $(LGENERAL_BUILD) -B $(LGENERAL_BUILD) \
		-DCMAKE_BUILD_TYPE= \
		-DCMAKE_C_FLAGS='$(CMAKE_C_FLAGS)' \
		-DCMAKE_CXX_FLAGS='$(CMAKE_CXX_FLAGS)' \
		-DLGENERAL_TRACE=$(TRACE_CMAKE) $(MIXER_CMAKE_ARGS) $(LGENERAL_CMAKE_ARGS)
	cmake --build $(LGENERAL_BUILD) -j$(JOBS)
	@echo "==> Staging translations"
	@set -e; for dom in lgeneral pg; do \
		for lang in en de; do \
			gmo="$(LGENERAL_BUILD)/po/$$dom/$$lang.gmo"; \
			if [ -f "$$gmo" ]; then \
				dir="$(GAME_DATA)/share/locale/$$lang/LC_MESSAGES"; \
				mkdir -p "$$dir"; \
				cp "$$gmo" "$$dir/$$dom.mo"; \
			else \
				echo "warning: missing catalog $$gmo" >&2; \
			fi; \
		done; \
	done
	@printf '%s\n' '@' 'grid=0' 'tran=1' 'bar=1' 'width=800' 'height=600' \
		'fullscreen=0' 'anim_speed=1' 'supply=1' 'weather=1' \
		'fog_of_war=1' 'cpu_turn=1' 'deploy=1' 'purchase=1' \
		'ai_debug=0' 'sound_on=1' 'sound_volume=96' 'music_on=1' \
		'music_volume=96' > $(GAME_DATA)/lgeneral.conf
	@set -e; \
	if [ -d "$(DATA_STAGE)" ]; then \
		for dir in $(GAME_DATA_DIRS); do \
			if [ -d "$(DATA_STAGE)/$$dir" ]; then \
				rm -rf "$(GAME_DATA)/$$dir"; \
				cp -a "$(DATA_STAGE)/$$dir" "$(GAME_DATA)/$$dir"; \
			fi; \
		done; \
	fi

lgeditor: $(LGEDITOR_BUILD)/.synced
	@echo "==> Building $(LGEDITOR)"
	cmake -S $(LGEDITOR_BUILD) -B $(LGEDITOR_BUILD) \
		-DCMAKE_BUILD_TYPE= \
		-DCMAKE_CXX_FLAGS='$(CMAKE_CXX_FLAGS)' \
		-DLGENDIR=$(GAME_DATA) $(LGEDITOR_CMAKE_ARGS)
	cmake --build $(LGEDITOR_BUILD) -j$(JOBS)

data: lgeneral
	@echo "==> Converting Panzer General data"
	@set -e; \
	if [ -z "$(strip $(PG_DATA))" ]; then \
		echo "PG_DATA is empty; skipping data conversion" >&2; \
	else \
		if [ -d "$(PG_DATA)" ]; then \
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
		rm -rf "$(DATA_STAGE)"; \
		mkdir -p "$(DATA_STAGE)"; \
		for dir in $(GAME_DATA_DIRS); do \
			if [ "$$dir" != convdata ] && [ -d "$(LGENERAL_BUILD)/src/$$dir" ]; then \
				cp -a "$(LGENERAL_BUILD)/src/$$dir" "$(DATA_STAGE)/$$dir"; \
			fi; \
		done; \
		mkdir -p "$(DATA_STAGE)/convdata"; \
		cp -a $(LGENERAL_BUILD)/lgc-pg/convdata/. "$(DATA_STAGE)/convdata"; \
		cd "$(DATA_STAGE)" && \
			$(LGENERAL_BUILD)/lgc-pg/lgc-pg -s "$$source_dir" -d .; \
	fi
	@set -e; \
	if [ -d "$(DATA_STAGE)" ]; then \
		for dir in $(GAME_DATA_DIRS); do \
			if [ -d "$(DATA_STAGE)/$$dir" ]; then \
				rm -rf "$(GAME_DATA)/$$dir"; \
				cp -a "$(DATA_STAGE)/$$dir" "$(GAME_DATA)/$$dir"; \
			fi; \
		done; \
	fi

# The game looks up data (and its config file) relative to the working
# directory, so it must be started from the data directory.
run:
	cd $(GAME_DATA) && ./lgeneral

# Build both variants from a pristine copy of the sources and require a
# warning-free result. Uses PG_DATA from the environment if set.
smoke:
	@tests/build-smoke.sh

clean:
	rm -rf $(BUILD_ROOT)

distclean: clean
