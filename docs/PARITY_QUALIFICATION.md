# MediEvil first parity review

Branch `feat/medievil1-parity-hle-20261006` starts at the existing native-quality
milestone `84202de`; the original owner checkout is untouched. Shared framework
is frozen at `20b0ea04`, UI at `03d58aa0`. Tracking: `beads-eio.18.8`.

The candidate retains OpenGL/1080p, mod-owned PGXP/perspective correction,
stable world filtering with sharp untracked UI, native Display interpolation,
adaptive native-wide projection, 3x terrain distance, subdivision and guarded
camera-plane clipping. Historical Crypt evidence is in ENHANCEMENTS.md; it
does not qualify the new combined build or the rest of the game.

## Execution selection and title boundary

Configure `-DPSX_EXECUTION_PROFILE=ENHANCED` (default), or use a separate build
directory with `REFERENCE`. The shared framework supplies its execution receipt
and fixed implementation selection. This title adds no loader HLE family.
Draw replay is presentation: both profiles continue executing original game
simulation and drawing. Generic image blending is not the native interpolation.

The guarded drawing section is `800239BC..80023A20`, after CD streaming,
effects and texture timers. Entry checks word `0C018431`, scratchpad bank
`1F800004` in [0,1], and the display pointer at `gp+3EC` in RAM. Submission
`8009BC64` requires word `27BDFFC0` and caller return `8002369C`. The replay
sandbox owns temporary CPU/RAM/GTE/VRAM changes; the original simulation is
restored. The extra pass invokes the original swap routine and DrawSync at
`8009C40C`. Cuts, incomplete captures and unmatched geometry retain current pose.

The owned USA code inventory contains MEDIEVIL.EXE plus 26 uncompressed
OVERLAYS/*.BIN images, with 29 AOT recipes including variants. There is no code
overlay decompressor to replace for this revision. Asset-container loading has
no established native adapter here. Faster loading and broader residency remain
open; existing native fallback handles code outside the offline inventory.

## Build and artifact handoff

Historical legal inputs, generated AOT and Windows builds are under
`F:/Projects/psxrecomp/_medievil-quality-20261003/medievil`. Reuse inputs only
after checking emitter/config/profile provenance. The campaign emitter is
`_build-fwtests-7b253941-20261006-sol-recompiler/psxrecomp-game.exe`,
CODEGEN 18 / d1867bb4. Refresh AOT against `aot/overlays.json`; the existing
profile and its missing-file flags do not prove complete game coverage.

Configure this worktree with Release, `PSXRECOMP_ROOT` pointing to the frozen
campaign framework, exact `RECOMP_UI_ROOT` and `RECOMP_RBENGINE_ROOT`,
`PSXRECOMP_REQUIRE_GAME_C=ON`, `PSX_PGXP_VARIANT=OFF`, `PSX_SDL_BACKEND=SDL3`,
and `PSX_EXECUTION_PROFILE=ENHANCED`. Build `psx-runtime --parallel 2`.
Use separate output directories for REFERENCE and native Linux.

`scripts/package_release.sh <build> <artifact> <native-emitter-build> ...`
delegates ZIP staging and final executable binding to the shared framework.
Supply the current platform's verified overlay cache through shared arguments;
do not substitute an older Windows cache in Linux. `--stage-only` prepares a
validated payload for the shared `tools/package_appimage.sh`; Linux packaging
and a title launcher remain pending the native build. No AppImage is claimed.

Source checks only: TOML, Bash syntax and diff hygiene. Windows compilation,
new AOT evidence and owner playtest are queued. Existing multiplayer behavior
is preserved; this is a single-player title.

## Owner check (about five minutes)

1. Start New Game, reach Dan's Crypt, and move toward the gate.
2. Rotate the camera at normal aspect and your widest display setting. Watch
   nearby walls/floors for holes, texture movement and subdivision seams.
3. Check health, dialogue and pause/resume; compare Native and Display smoothing.
4. Leave the Crypt, revisit, then save/load within this candidate if convenient.

Report the candidate, area and aspect setting with the visible issue. Steady
Display-rate delivery, all levels, loading gains and platform acceptance remain
open until reviewed.

## Resumed first-review build

The Windows review build now targets framework e5e2dca8 (20b0 plus the shared
Ninja fix for generated contract inputs), UI03d58aa0, and unchanged CODEGEN18
/d1867bb4. Earlier source inventories above retain their historical pins.
Per the owner, build ENHANCED and its native cache, then use the short manual
playtest before broadening tests. This preparation is not gameplay acceptance.
