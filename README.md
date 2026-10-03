<p align="center"><a href="https://alexbeavs-ps1-ports.github.io/psxrecomp-ports/"><img src="https://raw.githubusercontent.com/alexbeavs-ps1-ports/psxrecomp-ports/main/docs/assets/alexbeav-ps1-recomps-banner.png" alt="Alexbeav's PS1 Recomps" width="100%"></a></p>

# MediEvil Recompiled

<!-- retcomm-readme-metrics -->
[![GitHub downloads (all assets, all releases)](https://img.shields.io/github/downloads/Alexbeav/medievil-recomp/total)](https://github.com/Alexbeav/medievil-recomp/releases)
[![GitHub downloads (latest release)](https://img.shields.io/github/downloads/Alexbeav/medievil-recomp/latest/total)](https://github.com/Alexbeav/medievil-recomp/releases/latest)
[![GitHub release](https://img.shields.io/github/v/release/Alexbeav/medievil-recomp)](https://github.com/Alexbeav/medievil-recomp/releases/latest)
<!-- /retcomm-readme-metrics -->

Static recompilation of **MediEvil** built on
[psxrecomp](https://github.com/mstan/psxrecomp) and
[recomp-ui](https://github.com/mstan/recomp-ui).

MediEvil recompiled for modern systems using psxrecomp.

On the enhancement branch, **MediEvil Adaptive View** defaults to **Fit to
Window**. Change its View option in the launcher's Mods settings for 4:3,
16:9, 21:9 or 32:9. Fit follows wider window shapes with a 4:3 minimum;
movies keep their original proportions. The branch uses bundled OpenBIOS
with the BIOS shell skipped. Visual defaults include the 1080p internal
resolution preset, PGXP geometry correction and perspective textures. Adaptive
View also offers Original, 2x (default) and 3x terrain distance, plus a default-off
experimental terrain subdivision bypass. See [enhancement validation](docs/ENHANCEMENTS.md)
for the tested route and remaining qualification work.

| | |
|---|---|
| Players | 1 |
| Region | USA |
| Publisher | Sony Computer Entertainment |
| Year | 1998 |

Scaffolded with the New Project Layout. See
`psxrecomp/docs/GAME_PROJECT_SETUP.md` for the full flow.

<!-- retcomm-readme-launcher -->
## RetComM Launcher

You can run this title **standalone** (release zip + the built-in recomp-ui
Generate & Build flow), or manage installs, updates, ROM/BIOS wiring, and queued
builds more intuitively with
**[RetComM Launcher](https://github.com/TechnicallyComputers/RetComM-Launcher)** —
the Retro Compilation Manager hub for self-compiling recomps.

[Downloads](https://github.com/TechnicallyComputers/RetComM-Launcher/releases) ·
[Full README & features](https://github.com/TechnicallyComputers/RetComM-Launcher#readme)

<p align="center">
  <img src="https://raw.githubusercontent.com/TechnicallyComputers/RetComM-Launcher/main/docs/screenshots/hub-and-game-launcher.png" alt="RetComM hub with a background build, next to a title’s recomp-ui launcher" width="720">
</p>

<p align="center">
  <img src="https://raw.githubusercontent.com/TechnicallyComputers/RetComM-Launcher/main/docs/screenshots/queue-and-background-build.png" alt="Background cmake build with titles queued" width="720">
</p>

RetComM checks for updates, rebuilds with existing build data when possible,
uses the same platform build tools as per-title launchers, and automates
BIOS/ROM/save plumbing so you are not stuck repeating each game’s wizard by hand.
<!-- /retcomm-readme-launcher -->

## Legal

You must own the original game. Disc images under `disc/` are gitignored and
must never be committed. This development branch includes the MIT-licensed
OpenBIOS as its default; an owned SCPH-1001 BIOS remains an optional alternative.
The default skips the BIOS boot animation while retaining OpenBIOS kernel
initialization and EXE loading. BIOS kernel-call HLE remains disabled.
OpenBIOS has passed intro and main-engine startup checks. Full gameplay and
save/load qualification remain pending. Retail BIOS dumps are not redistributed.

## License

Project-owned source, scripts, configuration, and documentation use
`GPL-3.0-only`. See `LICENSE`.

This license does not cover MediEvil content, generated retail code, artwork,
names, or trademarks. PSXRecomp remains under PolyForm Noncommercial 1.0.0.
`recomp-ui` remains under MIT. See `THIRD_PARTY_NOTICES.md` and each submodule
license.

Default app icon: `assets/psxrecomp.ico` (and `.png` / `.svg`) — RetComM-themed controller mark from `psxrecomp/assets/`. Windows builds embed it via `APP_ICON`.

Optional box art under `launcher_assets/img/` may come from
[libretro-thumbnails](https://github.com/libretro-thumbnails/libretro-thumbnails)
(`Named_Boxarts`); see `BOXART_SOURCE.txt` when present.

## Quick start (dev)

```bash
git submodule update --init --recursive
./psxrecomp/tools/ci/build_emitters.sh
python3 psxrecomp/psxrecomp_cli.py generate \
  --config game.toml --project-root . --disc disc/<your>.cue
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target psx-runtime
```

To use your retail BIOS instead, add `--bios /path/to/SCPH1001.BIN` to Generate
and select that BIOS in the launcher.

Zip prefix for CI artifacts: `medievil-recomp`.

## Validation

The source repository keeps release evidence in `docs/VALIDATION.md`. A build
is not a public release until its exact package passes every listed gate. A
headless boot test does not replace a full gameplay test.

## Symbols

Progressive map: `symbols.toml` → `python3 tools/sync_symbols.py` →
`psx_symbols.h` (`PSX_FN_*`). See `psxrecomp/docs/SYMBOLS.md`.

## Framework pins

Submodule gitlinks (`psxrecomp`, optional `recomp-ui`, nested `recomp-net`)
are authoritative. `framework_pins.txt` is an optional scaffold snapshot;
release CI logs SHAs with `record_pins.sh` but builds whatever the gitlinks
resolve to. Bump submodules deliberately — do not float on `main`/`master`
in release CI.

## About this project

These ports are developed by a hobbyist (a DevSecOps engineer, not a game
programmer) with substantial AI assistance. Every change is validated before
it ships. The checks include boot gates, hardware-oracle comparisons,
deterministic probes, and a shared findings registry. AI writes most of the
code. The evidence decides what stays. Bug reports are welcome.

In short: AI writes the code, but I always test it before I publish it.

<!-- retcomm-readme-raid -->
---

<p align="center">
  <sub><b>R.A.I.D. — Retro AI Development</b> · a Discord for AI-assisted retro reverse-engineering, decomp &amp; recomp</sub>
</p>

<p align="center">
  <a href="https://discord.gg/Ad9BwSzctP"><img src=".github/raid-discord.png" alt="Join the Retro AI Development (R.A.I.D.) Discord" width="200"></a>
</p>
<!-- /retcomm-readme-raid -->

## v0.1.1 three-platform candidate

This candidate targets Windows x64, Linux x64, macOS ARM64, and macOS x64.
These setup packages require your legally owned game disc and a supported
regional retail BIOS. They remain unpublished until their exact package tests
and release authorization pass.
