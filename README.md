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
View also offers Original, 2x and 3x (default) terrain distance, with terrain
subdivision bypass enabled by default. Smooth Presentation follows the display
refresh by default, with 60/120/144/240/360 FPS choices. These are presentation
targets using camera/model draw interpolation; the game retains its original timing.
Stable world texture filtering is enabled by default, with nearest and bilinear
also available in Mods. HUD sprites retain sharp nearest sampling.
Live throughput can fall below the selected rate. See [enhancement validation](docs/ENHANCEMENTS.md)
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

## Enhancement foundation: OpenBIOS and native overlays

This development branch defaults to bundled MIT-licensed OpenBIOS and skips
the BIOS shell while retaining its kernel initialization and services. A retail
BIOS is optional. The historical package requirement above does not apply to
this branch.

The verified USA disc's main engine and 26 uncompressed overlays now have
explicit ahead-of-time compilation recipes. Generated code and disc assets
remain outside Git. PGXP tracking is compiled for subsequent visual mods.
Windows Release builds and OpenBIOS intro/main-engine startup checks pass;
full-game and packaged cross-platform qualification remain pending. See
[implementation and validation](docs/ENHANCEMENTS.md).

## Enhancement: adaptive world rendering

MediEvil Adaptive View is a default-on mod. Fit to Window reveals additional
world geometry at the current aspect ratio, with a 4:3 minimum; fixed 4:3,
16:9, 21:9 and 32:9 are available in Mods. Movies keep their original aspect
and the HUD retains its authored scale.

The title adapter expands bounded render storage and terrain capture. Shared
instruction-guarded culling and packet-validated horizontal projection recovery
address polygons disappearing or folding at wide boundaries. Stock vertical,
depth and backface checks remain. Dan's Crypt, dialogue, player movement and
fixed/adaptive aspect changes have been tested with OpenBIOS and OpenGL.
Remaining levels and complete title-scene boundary coverage need qualification.

## Enhancement: sharper and more complete terrain

The 1080p internal-resolution preset uses integer 5x rendering from the game's
240-line reference. PGXP geometry precision and perspective textures reduce
polygon wobble and texture warping.

Adaptive View offers Original, 2x and 3x terrain distance, with expanded fog
storage and bounded capture/primitive arenas. Conservative capture retains
tall walls outside the original ground footprint, and saturation-aware winding
checks recover terrain the original screen-coordinate tests would discard.
The subdivision bypass is selectable independently of draw distance.

Windows Release/diagnostic builds and contract tests pass. Live Crypt testing
fills missing walls and ceiling, including the room beyond the gate. Both 2x
and 3x distance and the subdivision option have been exercised. Extreme-aspect
title-scene black regions and other levels remain qualification work.

## Enhancement: default-on visual mods and display-rate presentation

| Control | Default | Where to change it |
| --- | --- | --- |
| Adaptive world view | Fit to Window | Mods: MediEvil Adaptive View |
| Terrain distance | 3x | Adaptive View: Original / 2x / 3x |
| Terrain subdivision bypass | On | Adaptive View: Bypass terrain subdivision |
| PGXP geometry and perspective textures | On, CPU propagation on | Mods: MediEvil PGXP Precision |
| Smooth Presentation | Display | Mods: Display / 60 / 120 / 144 / 240 / 360 FPS |
| Internal resolution | 1080p preset | Display settings; integer 5x at the 240-line reference |

Smooth Presentation replays the game's drawing code with interpolated camera
and model transforms inside a machine-state sandbox. Gameplay, input, timers
and audio keep their original speed. Display follows measured monitor refresh;
fixed choices set presentation targets. Draw cost limits the actual number of
extra renders. Cuts, unmatched geometry and UI retain the original game frame.
World Texture Filtering offers nearest, bilinear and stable minification. Stable
filtering is palette-aware and preserves cutouts, texture windows and sharp UI.

Both terrain subdivision selections are compiled ahead of time from separately
verified patched engine images. Selecting bypass no longer invalidates engine
code at startup. PGXP uses the shared plugin behind a title default-on manifest;
turning the mod off restores the base correction settings on the next launch.
Changes to Mods take effect when the game starts again.

These defaults target visual quality while retaining the game's simulation
cadence. Starting-room/hallway visibility, frame pacing and PGXP mod ownership
are validated in the enhancement receipt; remaining title/outdoor boundary
coverage and full-game save/audio checks are still documented there.
