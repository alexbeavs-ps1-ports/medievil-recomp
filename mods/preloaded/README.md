# Preloaded mods

Ship reviewed packages here. MediEvil Adaptive View is enabled by default and
offers Fit to Window, 4:3, 16:9, 21:9 and 32:9 in its View option. Fit retains a
4:3 minimum. Other enhancements retain their own defaults.

```text
packages/<package-id>/<version>/
  manifest.toml
  ...
```

Build wiring copies `mods/preloaded` next to the game executable as `mods/`.
Install player `.psxmod` archives through the launcher Mods manager instead of
committing them here. See `psxrecomp/docs/MOD_PACKAGES.md`.
