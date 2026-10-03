# Preloaded mods

Ship reviewed packages here. MediEvil Adaptive View is enabled by default and
offers Fit to Window, 4:3, 16:9, 21:9 and 32:9 in its View option. Fit retains a
4:3 minimum. Terrain distance defaults to 3x, with Original and 2x options;
terrain subdivision bypass defaults on. Both choices have verified native AOT
images, without runtime executable mutation.

MediEvil Smooth Presentation defaults on at Display refresh, with
60/120/144/240/360 FPS choices. It uses shared motion-adaptive frame blending
with a real-frame-flip source and retains original simulation timing.

The title overrides the shared PGXP package manifest with geometry/perspective
correction and CPU propagation enabled by default, using the same shared plugin.
Internal resolution remains a Display setting and defaults to the 1080p preset.

```text
packages/<package-id>/<version>/
  manifest.toml
  ...
```

Build wiring copies `mods/preloaded` next to the game executable as `mods/`.
Install player `.psxmod` archives through the launcher Mods manager instead of
committing them here. See `psxrecomp/docs/MOD_PACKAGES.md`.
