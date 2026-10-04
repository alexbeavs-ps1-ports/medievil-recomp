# Preloaded mods

Ship reviewed packages here. MediEvil Adaptive View is enabled by default and
offers Fit to Window, 4:3, 16:9, 21:9 and 32:9 in its View option. Fit retains a
4:3 minimum. Its terrain distance defaults to 2x, with Original and 3x options.
The experimental terrain subdivision bypass defaults off. Internal resolution
defaults to the 1080p preset, with PGXP geometry and perspective textures on.

```text
packages/<package-id>/<version>/
  manifest.toml
  ...
```

Build wiring copies `mods/preloaded` next to the game executable as `mods/`.
Install player `.psxmod` archives through the launcher Mods manager instead of
committing them here. See `psxrecomp/docs/MOD_PACKAGES.md`.
