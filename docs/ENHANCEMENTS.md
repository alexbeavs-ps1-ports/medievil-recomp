# MediEvil enhancement assessment and first implementation

Assessment date: 2026-10-02. Branch: `feat/medievil-enhancements`, based on
Alex's main `f8eb216f10ef7f644b33643330b967cd29562709`.

## What the two ports provide

BlackLabel's port uses RecompOne (C#/.NET, with BIOS and named PsyQ SDK HLE).
Its [v0.1b release](https://github.com/BlackLabelHQ/MediEvilRecomp/releases/tag/v0.1b)
advertises widescreen, adjustable aspect and draw distance, PGXP, revised terrain
rendering and frame interpolation. Completion/chalice/item support is the
authors' claim; this assessment did not playtest that build. The authors also
report audio and PGXP/interpolation imperfections.

Alex's existing port is a PSXRecomp standup using retail BIOS execution, a
4:3 OpenGL configuration, digital input and shared launcher/setup/save surfaces.
Its original source compiles the boot EXE, with no explicit engine/level AOT
profile or MediEvil enhancement plugin. The original validation receipt covers
a 25-second hidden startup, not a complete gameplay route. v0.1.3 primarily fixes
the setup executable name; its publication does not establish gameplay quality.

BlackLabel source reviewed at `6daae8f78708dd848162b7ac617991f2efb7d639`,
RecompOne at `8d0091cf4877b27c2d526106a78c75f6b0ef97b2`.

## Reuse boundary

| Capability | Current PSXRecomp | Work belongs in |
| --- | --- | --- |
| PGXP and perspective textures | Already shared; complete tracking needs the PGXP build | Framework, enabled by title build/UI |
| Adaptive native-wide rendering | Shared host/renderer surface used by Tomba | Framework plus a MediEvil world/HUD/culling adapter |
| More guest RAM | Shared optional 8 MiB map | Framework; buffer relocation/capacity changes are engine-specific |
| Offline engine/overlay compilation | Declarative AOT pipeline already exists | Title profile; reusable extraction methods in framework |
| Draw distance, terrain, fog, ordering-table capacities | No universally safe slider | MediEvil engine plugin; common helpers where another engine can reuse them |
| Transform-aware geometry interpolation | No RecompOne-style generic transform matcher today | Framework provenance, matching and replay, with title exceptions |
| Redraw-based interpolation | Shared render-pass sandbox exists | Title supplies scene/camera/object hooks |
| SDK HLE | Different execution strategy from the retail BIOS/PsyQ floor here | Optional future framework architecture, unnecessary for these enhancements |

RecompOne's GPU interpolation tracks transform identity/provenance from GTE
through PGXP, matches geometry across frames and interpolates rotation and
translation before reprojection. PSXRecomp's image-blending modes do not provide
that geometry. Its render-pass API is another reusable route but needs a game
draw adapter. Neither should be confused with increasing gameplay simulation
speed.

BlackLabel's wider terrain patch enlarges primitive/capture/ordering-table/fog
storage and changes clipping/subdivision. These are engine changes rather than
new terrain assets. Its unfinished LandMapPatch is not configured as an active
patch. The several-thousand-line CullPatch cannot be generalized by replacing
addresses alone.

## Owned USA disc and AOT

The owned two-track dump was copied locally into ignored `disc/`.
Data Track 1 is 528755472 bytes, SHA256
`d522a86c154524d634a0d9b84b2048b41cafe83013a87f2fa31a2c117aabce82`.
The boot EXE SHA256 is
`b1813ebff44a59d0a92937b6f920e8624b682b1807936d3daf40c5747347dea5`;
its serial/header and TOC fingerprint match the existing USA binding.

All 28 code-file SHA1 values match the
[public decomp configs](https://github.com/MediEvilDecompilation/medievil-decomp/tree/6afe6fe35d5ddf0ce1bebdb2e72f8215b5b5b407/config):

| Producer | Format | Load address | Generation |
| --- | --- | --- | --- |
| SCUS_942.27 | PS-X EXE | 0x801B0000 | Existing boot generation |
| MEDIEVIL.EXE | PS-X EXE, skip 0x800 header | 0x80021CA4 | AOT `psx_exe` |
| 26 OVERLAYS/*.BIN | Raw, uncompressed code/data | 0x80010000 | AOT `fixed_address_files` |

There is no overlay decompressor to implement for this verified revision.
This conclusion concerns code overlays; it makes no claim that every asset
container is uncompressed. Our X5/X6 profiles likewise use raw
`sector_extent_members` from ROCK_X5.BIN/ROCK_X6.BIN, not an LZSS decoder for
those code producers. MediEvil reuses the same pipeline with simpler inputs.

`aot/overlays.json` declares all 27 engine/overlay producers, exact disc gating,
strict bounds and SHA256-verified data/rodata exclusions derived from the decomp.
The generic extractor found only MEDIEVIL.EXE and HH.BIN; the fixed-file recipes
recover the other 25 without adding game-name logic to the framework.

The profile currently gates on the verified split dump. The previously accepted
combined dump remains accepted by setup, but it needs a separately verified AOT
binding before it receives static overlays; the pipeline fails closed for it.

## Implemented foundation

- Updated framework/UI gitlinks and the recorded pins/manifest; the framework
  URL uses the canonical upstream containing the selected AOT implementation.
- Added the PGXP build flag to the primary executable. The shared PGXP mod
  remains opt-in in the launcher; compiling tracking does not enable it.
- Connected generated static overlay C to the game target and included `aot/`
  in the setup package's source tree. Native fallback caching stays enabled.
- Added the verified split Track 1 sizes/digests to normal disc validation.
- Supplied recomp-net header declarations for the pinned runtime's launcher
  status type when netplay is off. This does not enable a network session.
- Kept retail rendering defaults while the adaptive widescreen adapter is built.

Generation command (from the project root, with a configured toolchain):

```powershell
python psxrecomp/psxrecomp_cli.py generate --config game.toml --project-root . --disc "disc/MediEvil (USA).cue"
```

Local CLI Generate passed, including CPS emission. The static audit validates
27 recipes and 48778 guarded variants in 29 generated C files, with every guard
matching known disc bytes. `full_static_coverage_proven` is deliberately false:
discovery and byte validity do not prove exhaustive indirect-entry coverage or
native execution correctness. Generated C, binaries, captures and BIOS stay
ignored and are not part of source commits.

The Windows x64 Release host build passed with static overlays linked and
`PSX_PGXP=1` / overlay flavor 2. A 25-second headless software-renderer run with
the owned disc and local SCPH-1001 BIOS remained alive and reported advancing
frames through 5131 at 23.054 seconds. The harness terminated it at its time
limit; its termination exit code is not a crash classification. Headless frame
counts are unpaced throughput, not a gameplay FPS claim.

The current framework emits warnings that its committed BIOS C has stale
emitter fingerprints. This build linked those committed BIOS backends; they
were not regenerated for this milestone. Visible gameplay, audio, save/load,
level transitions, OpenGL and PGXP visual comparisons remain unvalidated.

## Adaptive widescreen implementation direction

Use Tomba's custom native-wide renderer and adaptive aspect APIs as requested.
Tomba's current fit mode has a 4:3 floor, so support for narrower portrait ratios
would require additional renderer/aspect policy work. Wider arbitrary window
ratios need validation beyond the familiar 16:9/21:9/32:9 examples.

MediEvil must supply its own projection/culling sites, world-versus-HUD primitive
classification, backdrop handling and safe terrain/primitive capacity changes.
Do not transplant Tomba's guest addresses or advertise a working widescreen
toggle before those hooks are verified in the main engine and level overlays.
Start with visible boot and level-transition parity, then PGXP comparisons,
adaptive widescreen and terrain capacity checks. Treat transform interpolation
as its own shared framework project after those foundations are established.
