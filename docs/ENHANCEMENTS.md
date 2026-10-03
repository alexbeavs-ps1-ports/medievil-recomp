# MediEvil enhancement implementation notes

Updated: 2026-10-02. Branch: `feat/medievil-enhancements`, based on
Alex's main `f8eb216f10ef7f644b33643330b967cd29562709`.

## Starting point

Alex's original port is a PSXRecomp standup using retail BIOS execution, a
4:3 OpenGL configuration, digital input and shared launcher/setup/save surfaces.
Its original source compiles the boot EXE, with no explicit engine/level AOT
profile or MediEvil enhancement plugin. The original validation receipt covers
a 25-second hidden startup, not a complete gameplay route. v0.1.3 primarily fixes
the setup executable name; its publication does not establish gameplay quality.

## Reuse boundary

| Capability | Current PSXRecomp | Work belongs in |
| --- | --- | --- |
| PGXP and perspective textures | Already shared; complete tracking needs the PGXP build | Framework, enabled by title build/UI |
| Adaptive native-wide rendering | Shared host/renderer surface used by Tomba | Framework plus a MediEvil world/HUD/culling adapter |
| More guest RAM | Shared optional 8 MiB map | Framework; buffer relocation/capacity changes are engine-specific |
| Offline engine/overlay compilation | Declarative AOT pipeline already exists | Title profile; reusable extraction methods in framework |
| Draw distance, terrain, fog, ordering-table capacities | No universally safe slider | MediEvil engine plugin; common helpers where another engine can reuse them |
| Transform-aware geometry interpolation | Generic transform provenance/matching/replay remains to be implemented | Framework, with title exceptions |
| Redraw-based interpolation | Shared render-pass sandbox exists | Title supplies scene/camera/object hooks |
| SDK HLE | Original SDK code executes through the runtime today | Shared optional backends, qualified against the existing API contract |

Geometry interpolation needs transform identity/provenance from GTE through
PGXP, geometry matching across frames, and rotation/translation interpolation
before reprojection. The existing image-blending modes do not provide that
geometry. The render-pass API offers another route through a game draw adapter.
Keep gameplay simulation cadence intact when adding intermediate presentation
frames.

Wider and farther terrain rendering needs safe primitive/capture/ordering-table/
fog storage, plus appropriate clipping and subdivision behavior. These changes
belong to the MediEvil engine adapter. Shared host helpers should expose reusable
behavior rather than embed this game's addresses or buffer layout.

## BIOS and PsyQ HLE

HLE means high-level emulation: host code implements a service's behavior
instead of executing the original instructions. There are two boundaries:

- BIOS HLE replaces kernel calls, such as event, file, thread or memory-card
  services, behind the existing guest-facing kernel interface.
- PsyQ SDK HLE replaces recognized library routines linked into the game itself.
  Candidate boundaries include CdRead, DrawOTag, VSync and PadInitDirect.
  Function identity and the applicable SDK variant must be verified before
  installing a replacement; arbitrary custom game code stays on its existing
  execution path.

HLE can reduce SDK/device work and provide useful host integration boundaries
for loading, rendering and input. No comparative performance benchmark has been
run for this port. Matching API behavior remains necessary for games that patch
kernel internals, depend on asynchronous completion or use custom SDK routines.

OpenBIOS is a different choice: an open-source PS1 BIOS is compiled and executed
through PSXRecomp, preserving the game's SDK path while removing the requirement
for a retail ROM. PSXRecomp already has a narrower optional BIOS HLE tier and a
host scheduler. Its OpenBIOS backend declines the event-call HLE tier, so those
kernel services execute in OpenBIOS; boot-shell skipping is independently
available. BIOS selection and SDK replacement are separate decisions.

## HLE replacement contract

Owner direction: HLE may replace LLE components when it maintains a reasonable
API contract with the existing path. Implement replacements behind the same
guest-facing interface and retain the LLE path as a reference and fallback.

The contract covers arguments, return values, errors, register preservation,
guest-memory writes and observable device state. Preserve callback identity and
ordering, interrupt delivery, busy/completion states, and blocking versus
nonblocking behavior. Maintain guest scheduling and frame cadence; matching the
original internal instruction sequence is unnecessary when observable behavior
is preserved. Any deliberate timing enhancement needs an explicit option and
documented behavior.

For example, a host implementation of CdRead must write the expected guest
buffer and preserve command status, error and completion/callback behavior.
DrawSync must still report or wait for the appropriate completion state. VSync
must honor its query/wait modes and guest cadence even if presentation runs at a
higher refresh rate.

Keep state deterministic and include backend state in save/restore or rollback
when those services use it. Validate verified call sequences against the LLE
reference, comparing guest-visible outputs, side effects and event ordering.
Document tolerances for observable timing rather than assuming a host-fast
return is compatible. Unsupported API variants or failed identity checks stay
on LLE.

Put reusable API adapters, identity checks and backend selection in PSXRecomp;
keep MediEvil addresses and activation policy in this port. Begin with a bounded
service and its contract instead of replacing entire subsystems at once.

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
- Added the PGXP build flag to the primary executable. Geometry correction,
  perspective textures and CPU provenance tracking now default on.
- Connected generated static overlay C to the game target and included `aot/`
  in the setup package's source tree. Native fallback caching stays enabled.
- Added the verified split Track 1 sizes/digests to normal disc validation.
- Supplied recomp-net header declarations for the pinned runtime's launcher
  status type when netplay is off. This does not enable a network session.
- Added the default adaptive view and visual defaults described below.

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

## OpenBIOS development default (2026-10-02)

The inherited retail-only setting had no documented MediEvil-specific failure
in the inspected standup receipts. This branch now allows OpenBIOS, removes the
retail-only packaging flags and documents the bundled MIT notice. Generate
succeeds without a BIOS argument. An explicitly selected owned SCPH-1001 ROM
remains available; BIOS call HLE stays off in the title configuration.

Two 25-second Release probes passed with OpenBIOS: normal boot and shell-skip.
Both logs identify OPENBIOS; the latter explicitly retains LLE kernel services.
These frame counters alone are only liveness evidence.

A separate diagnostic build (Clang 22.1.8, Release/O1, PGXP tracking and debug
tools enabled) compared OpenBIOS and SCPH-1001 using the same owned disc and
isolated save directories. Both displayed the introductory movie, then the
story text and dungeon intro after Start input. By 30 seconds, live main-engine
bytes at 0x80021CA4 matched the disc. At 40 seconds the OpenBIOS run reported
1100 distinct engine entries used through static AOT dispatch and the retail
run reported 1102. These are observed entries, not exhaustive coverage or a
speed comparison; the samples were not aligned by guest frame.

The default-selection probe uses the actual game.toml and omits --bios, rather
than forcing an OpenBIOS path. It selected the bundled OPENBIOS image and
reached the same story/dungeon intro route, with matching engine bytes and 1100
observed native engine entries by 40 seconds. Screenshots and receipts remain
private under ignored analysis/. Full gameplay, audio and save/load qualification
remain pending before any public release. Committed upstream BIOS backends were
used; the earlier emitter-fingerprint warning still applies.

## OpenBIOS fast-boot playtest correction (2026-10-02)

A live upstream query confirmed that framework HEAD/master remains
`641537be8210f96f61a8ad69d6021844b56e30e1`, already pinned by this branch.
There is no newer framework gitlink to adopt for this correction.

The interactive launcher had persisted an explicit SCPH-1001 path in
settings.toml and bios.cfg, overriding the title's OpenBIOS default, and had
disabled fast_boot. The local preferences were backed up, that BIOS selection
cleared, and boot skip enabled while preserving other preferences and saves.
The local playtest shortcut now explicitly selects the bundled OpenBIOS.

The title now defaults to runtime.fast_boot=true with bios_hle=false. This
skips the BIOS shell through the shared boot intercept while retaining the
recompiled OpenBIOS kernel initialization and EXE loading. It does not enable
BIOS kernel-call HLE; other existing runtime HLE services have separate policy.

Generate passed with 27 verified recipes and 48778 guarded variants; the
Windows Release rebuild passed. A 25-second headless probe using the corrected
real settings and no BIOS command-line override reported image=OPENBIOS,
bios_backend=LLE and bios_boot=HLE (shell skipped), with advancing frames.
The rebuilt interactive OpenGL playtest was then launched explicitly with
OpenBIOS and confirmed the same BIOS/backend/boot mode in its startup log.
Its visible game window was responsive and presented advancing frames. Full
gameplay, audio and save/load qualification remain pending.

## Adaptive native-wide view (2026-10-02)

The preloaded MediEvil Adaptive View package now defaults to Fit to Window.
It uses the shared native-wide renderer used by Tomba, with MediEvil-specific
capture and culling hooks. Its View option also offers 4:3, 16:9, 21:9 and 32:9.
Fit follows wider window ratios continuously and retains the renderer's 4:3
minimum. Movies retain their authored aspect. HUD and dialogue retain their
original scale and remain within the central frame; separate edge anchoring
is not enabled.

The initial framework pin was local commit `c29e93430814288f05b05224bb232b923daf586a`
on `feat/guarded-packed-wide-cull`, extending the latest checked upstream master
`641537be8210f96f61a8ad69d6021844b56e30e1`. The shared addition supports
full-instruction-guarded packed-coordinate reject predicates, including native
code, cached-module callback ABI 26 and dirty-RAM interpretation. Expected
instructions and masks enter cache identity. At 4:3 the predicate is the
original reject; wide views preserve vertical flags and defer horizontal
clipping to the renderer. The local framework commit must be made available
to downstream checkouts before distributing this source branch.

The title supplies five guarded main-engine polygon-reject sites, a separate
TL overlay reject with an instruction unique across the owned overlays, and
widens the existing
object-box horizontal bounds while retaining vertical, depth and backface
tests. The live 3D viewport stores 512 pixels in scratchpad; its capture-plane
SVECs independently use a 320-unit span. The adapter converts the live margin
between those scales, including a 32-pixel culling guard.

Ten instruction-guarded disc patches relocate the original 100-entry marked
cell list into expanded RAM. The capture routine retains its own bounded count
check with a 1024-entry limit. Its 8-byte records and marked-cell pointers have
separate arenas, the polygon bookkeeping has 8192 slots, and two primitive
buffers each have 1 MiB with end slack. The original heap allocation and teardown
remain game-owned. Fog, view distance, subdivision and object activation remain
stock. The adapter changes no overlay compression or loader format.

The modified main engine is a separately byte-verified AOT producer, alongside
the original engine and 26 raw level overlays. Generate audited 28 recipes and
49253 guarded static variants. The extractor uses the owned original disc and
the exact selected package view; it needs no runtime captures. This inventory
does not prove exhaustive static coverage.

Visual testing caught a mesh-tearing error from the generic HUD corner
heuristic moving world polygons. It is disabled for this title. The corrected
OpenBIOS/OpenGL diagnostic build reached Dan's Crypt (CR overlay), displayed
the gargoyle dialogue and controllable character, and rendered additional room
geometry at 16:9, 21:9 and 32:9 without those tears. At a live 1260x600 resize,
Fit produced an 806x240 native surface from the 512x240 source; a narrower
900x700 window returned to the 4:3 floor. Capture corners and render arenas
updated during those transitions. Sampled unused bytes after the capture and
polygon lists remained zero. These samples are bounded route evidence, not
proof against overflow in every level.

Windows Release and diagnostic builds passed. Code-generation, real interpreter
and cached callback tests passed; 53 original-disc AOT method tests passed.
The standalone interpreter harness was built with GCC because its existing
Clang/LTO build retains unrelated GPU dependencies and fails to link. Screenshots
and live receipts remain private in ignored analysis/. Outdoor/boss overlays,
ratios beyond the tested cases, audio and save/load still need qualification.
The TL site was verified from original disc instructions; that level was not
included in the live route.

## Horizontal projection folding correction (2026-10-02)

The horizontal projection correction first landed in local commit
`3787120488cf343bb167b87b18e5b614aa752256`, extending the same upstream
`641537be8210f96f61a8ad69d6021844b56e30e1` baseline. It remains on
`feat/guarded-packed-wide-cull`. Its ancestry is published through framework
[PR #483](https://github.com/RetroPortingToolKit/psxrecomp/pull/483).

A Crypt diagnostic captured a terrain GT4 with vertices (882,117), (988,130),
(1023,152), (1023,173). Its two triangle signed areas were +1877 and -735:
two different camera-space vertices had collapsed onto the GTE's X=1023
saturation edge. The host renderer now recovers saturated horizontal positions
from exact packet-address/word-validated projection shadows when the native-wide
view is active. The title opts into this shared policy through its activation
plugin. Guest geometry registers, packet data, vertical coordinates, collision,
fog, subdivision and ordering-table behavior remain game-owned.

The executable GPU regression restores consistent triangle winding and checks
stock fallback for stale/missing provenance, missing depth, zero reveal and
transport bounds. A separate PGXP test verifies the real projection transport.
Existing GPU anchor/HUD regressions passed, and the Windows Release rebuild
passed. OpenBIOS/OpenGL Crypt A/B captures and lateral movement exercised the
correction at 32:9 and Fit in a 2048x490 window (1604x240 native surface).
Screenshots and receipts remain private under ignored analysis/. This establishes
the captured folding mechanism and live execution; it does not conclusively
match every part of the owner's reported left-wall deformation.

## Visual defaults and extended terrain (2026-10-02)

The current framework pin is `46af572902210d94e46305355ad22649c6b0bd3f`,
extending the native-wide integration merge `a95d8c77ee57d5aee84cc46142a0ac6f86d52cd7`
of upstream master `973d93a90761c8ef986ce9af63965d96a3613ed3`. Framework
[PR #483](https://github.com/RetroPortingToolKit/psxrecomp/pull/483) publishes the
rendering fixes; [PR #484](https://github.com/RetroPortingToolKit/psxrecomp/pull/484)
adds the PGXP session-ownership fix. The pins are available on the canonical
remote. The current dependency gitlinks resolve locally; a recursive fetch
encountered an unavailable historical netplay revision, so upstream was fetched
without recursing into its historical submodules.

| Setting | Default | Behavior |
| --- | --- | --- |
| Internal resolution | 1080p preset | Integer 5x scale from the 240-line reference, producing 1200 internal lines |
| PGXP mod | On | Shared plugin behind a title default-on manifest; geometry, perspective textures and CPU provenance, tolerance 1.0 |
| Adaptive view | Fit | Reveals more world at the window ratio, with a 4:3 minimum |
| Terrain distance | 3x | Mod option also offers Original and 2x |
| Terrain subdivision bypass | On | Conditional guarded disc patches; both selections compiled ahead of time |
| Smooth Presentation mod | Display | Shared motion-adaptive blending, with 60/120/144/240/360 presentation targets |

The terrain adapter uses verified USA function boundaries and retains the guest
capture-record, marked-cell cleanup and primitive-list contracts. It replaces
the ground-intersection capture footprint with a conservative camera-centered
XZ disk, so a tall wall is not discarded solely because its ground footprint
lies outside the original capture frustum. Cells are sorted nearest first,
deduplicated through the original capture flag and bounded by 2048 records and
8192 raw polygon references. Existing vertical, depth and backface tests still
run. Object activation, collision and simulation distance remain game-owned.

Fog occupies an expanded-RAM arena with 1024 entries and padding. The original
fog-parameter and animation routines still build the curve. Increasing the
viewport's SZ-to-ordering-table shift extends the existing table's reach;
the ordering-table allocation, size and terminators remain guest-owned. This
trades depth-bucket precision for reach. Teardown restores the original viewport
fields only when they still match the adapter's recorded values and never frees
the expanded arena through the guest heap. Bookkeeping resides in guest RAM for
timeline consistency. Repeated frames do not compound the distance multiplier.
Distance is bounded by the exclusive table/fog ceiling: the observed title
distance changes from 5632 to 11264, and Crypt from 4096 to 8191 at 2x.

The subdivision option changes only instruction-verified thresholds at
`0x80022108` and `0x8002279C`. Conditional declarative disc patches select it;
both patched engine images receive separate byte-verified AOT producers, so
selection does not invalidate executable RAM or move the terrain funnel into
fallback. The original OTZ >= 4 near rejection remains. The original renderer
can be selected by turning the option off; near-camera quality and remaining
level coverage still require qualification. Bypass now defaults on to match
the owner's visual-quality preference.

The shared renderer now also supplies full-instruction-guarded native-wide
NCLIP branch predicates for three terrain winding consumers. Saturated X can
make the original integer area zero or reverse its sign before the renderer
ever receives the polygon. The predicate uses fresh, packet-word-validated
projection provenance to recover winding, with near-plane, missing-depth,
stale-data and timeline fallbacks. Architectural NCLIP results and guest
registers remain unchanged. Codegen version 14 and overlay callback ABI 27
invalidate older cached code. The debug counter `ws_nw.nclip_rescues` counts
changed branch decisions rather than rendered primitives.

Windows Release and diagnostic builds pass. The terrain contract test exercises
fog ownership/restoration, noncompounding distance, tall-wall capture, duplicate
cell IDs, dense-grid budgets and absence of runtime executable mutation. Shared real-GTE,
interpreter, codegen and cached-callback tests pass, together with the 53-method
AOT suite and the new upstream ChangeThread-self and TCC include checks.
Live visual qualification is recorded below. Full visual parity is not claimed;
transform interpolation, outdoor/boss/TL coverage, audio and save/load still
require separate qualification.

The final OpenBIOS/OpenGL diagnostic route reached the title menu and Dan's
Crypt. Fit at a 2048x490 client area produced an 8020x1200 captured native-wide
surface at 5x scale. Live PGXP reported geometry correction, perspective
triangles and CPU tracking enabled at tolerance 1.0. Winding-rescue and saturated
projection counters advanced. Crypt captures filled the previously missing
walls and ceiling, including the room beyond the gate; lateral left movement,
4:3, 16:9, 21:9, 32:9 and Fit transitions remained responsive. This supports the
observed saturation/capture corrections, not a claim that every level or every
frame of the original deformation report has been reproduced.

A separate restart with 3x distance and subdivision bypass enabled reached the
Crypt. Both executable thresholds read back as `0x290A0000`, the Crypt far
distance was 12288, and the viewport used reach 16384/shift 2. Default 2x used
8191 and reach 8192/shift 1. Render records stayed within their arenas in the
sampled route. The subdivision option retained the near rejection and the live
test did not establish universal near-camera quality. Private mod state was
restored after the test. This was the earlier runtime-patch implementation;
the current default-on option uses independently compiled native images.

The final title-menu capture reveals additional scenery on the right but still
contains black background regions at extreme Fit ratios. Whether those are
authored scene boundaries or additional sky/model rejection paths is not yet
established. This remaining coverage is tracked under `beads-eio.18.5`; the
current fixes do not establish complete title/outdoor boundary coverage.

## Default-on mods and presentation cadence

PGXP activation previously set live flags before renderer initialization,
which then silently overwrote them with the base video configuration. The
shared session-owned precision API fixes that ordering and resets the override
before the next session's plan. The game keeps base geometry/texturing off and
ships a default-on override of the shared PGXP manifest; disabling the mod
therefore restores the base path. Previously task-added local video overrides
were removed without changing the player's other preferences.

Smooth Presentation uses the shared OpenGL motion-adaptive temporal blend and
the FLIP source: a 30 Hz game image can span two original guest VBlanks. Display
passes the zero sentinel to follow measured monitor refresh. Fixed 60, 120,
144, 240 and 360 choices select presentation targets. Gameplay, timers, input,
CD and audio keep their original cadence. This combines completed images;
it does not implement transform interpolation or motion-vector generation.
Large visual changes switch more cleanly, but temporal blending can soften
moving edges and retains a source-image delay. FMVs suspend blending.

The owned-disc audit now validates 29 images/recipes and 49330 guarded variants
in 31 generated files. Capture metadata at expanded guest RAM `0x80308C20`
records candidate cells, captured cells, raw polygon references and cells shed
by the budget, allowing room pop-in to be distinguished from budget exhaustion.
The owner narrowed the visibility report to the starting Crypt/intro area.
