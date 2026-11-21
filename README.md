# LGeneral2

A fork of [LGeneral](https://github.com/MrScruffyish/LGeneral), which mirrors
the upstream [LGeneral](https://lgames.sourceforge.io/LGeneral/) SVN project.

The source is split into two independent projects at the top level:

- `lgeneral/` — the game
- `lgeditor/` — the map editor

## Building

The top-level `Makefile` builds both sub-projects into `.build/`:

```bash
make
```

It needs CMake 3.16 or newer, `pkg-config`, SDL 1.2 with SDL_mixer (the
`sdl12_compat` packages work), SDL2 with SDL2_image and SDL2_ttf, and gettext
0.21 or newer. CMake runs inside an isolated `.build/` copy, so generated
files are never added to the source tree. To also convert the Panzer General
data into the game's data directory, set `PG_DATA` to `pg-data.tar.gz` or an
unpacked `pg-data` directory:

```bash
PG_DATA=~/PG_DATA/pg-data.tar.gz make
```

The executables end up in `.build/lgeneral/src/lgeneral`,
`.build/lgeneral/lgc-pg/lgc-pg` and `.build/lgeditor/src/lgeditor`. Start the
game with `make run`, or from its data directory:

```bash
cd .build/lgeneral/src && ./lgeneral
```

Running it from the repository root fails because the game looks up its data
and `lgeneral.conf` relative to the current directory.

### Tracing

For diagnosing data/deployment problems, build a traced copy:

```bash
PG_DATA=~/PG_DATA/pg-data.tar.gz TRACE=1 make
make TRACE=1 run
```

It builds into `.build/lgeneral-trace` and logs scenario loading, players,
reinforcements and deployment candidates to `/tmp/human.log`. Set
`LG_TRACE_FILE` to write somewhere else.

### Smoke test

`make smoke` copies the sources to a temporary directory and builds both the
normal and `TRACE=1` variants there. It fails on compiler warnings, missing
executables, or a trace build without trace messages, so it also serves as a
warning-free build check:

```bash
PG_DATA=~/PG_DATA/pg-data.tar.gz make smoke
```

It also checks that both gettext domains (`lgeneral` and `pg`) compile and that
a German string is actually returned at runtime.

### Translations

GNU gettext translations are enabled. The two catalogs in `lgeneral/po/` are
compiled to `.gmo` during the build and staged into
`.build/lgeneral/src/share/locale/<lang>/LC_MESSAGES/`. Select a language at
runtime with `LANGUAGE`, for example:

```bash
cd .build/lgeneral/src && LANGUAGE=de ./lgeneral
```

The catalogs and the game's bitmap font are ISO-8859-1, so `localize.c` binds
that codeset explicitly. `lgeneral.pot` is generated from the `tr()`, `trd()`
and `TR_NOOP()` calls in `lgeneral/src/`; run `make -C
.build/lgeneral/po/lgeneral update-po` after changing source strings, then
update the `.po` files.

## License

**GPL-3.0-or-later** (see `LICENSE`). The original upstream GPL v2 text is kept
verbatim in `LICENSE.GPLv2` and in `lgeneral/COPYING` / `lgeditor/COPYING`.
Third-party components keep their own licenses and attributions; see
`THIRD_PARTY_LICENSES.md`.

## Relationship to LGeneral

- `LGeneral` is the SVN mirror and the only place where `svn update` is run.
- `LGeneral2` is this fork, where development happens.
- Git history is genuinely shared: LGeneral's commits are ancestors of this
  repository's commits.
- Remotes:
  - `origin` — `MrScruffyish/LGeneral2` (this repo)
  - `upstream` — `MrScruffyish/LGeneral` (SVN mirror, SVN r204 / commit `3157441`
    at the time of forking)

## Getting SVN updates into this fork

Run SVN in the mirror, then bring the commits in here:

```bash
# In /home/gunnar/MrScruffy/LGeneral
(cd lgeneral-svn && svn update)
git add -A
REV=$(svn info --show-item revision lgeneral-svn)
git commit -m "Sync SVN trunk to r$REV"
git tag "svn/r$REV"
git push origin main --tags

# In this repo (LGeneral2)
git fetch upstream
git merge upstream/main
git push origin main
```

Note: this fork moved `lgeneral-svn/lgeneral` and `lgeneral-svn/lgeditor` to the
top level, so upstream changes to those files may produce rename/modify
conflicts. Git often follows the rename automatically, but heavy local changes
will require resolving conflicts manually (or cherry-picking individual commits).

Do **not** run `svn update` inside this repository. The leftover
`lgeneral-svn/.svn/` metadata is kept for reference only; the SVN working copy
itself no longer exists here because the source directories were moved.
