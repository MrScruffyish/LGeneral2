# LGeneral2

A fork of [LGeneral](https://github.com/MrScruffyish/LGeneral), which mirrors
the upstream [LGeneral](https://lgames.sourceforge.io/LGeneral/) SVN project.

The source is split into two independent projects at the top level:

- `lgeneral/` — the game
- `lgeditor/` — the map editor

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
