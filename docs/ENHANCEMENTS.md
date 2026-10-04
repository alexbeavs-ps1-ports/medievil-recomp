# MediEvil enhancement implementation notes

Current framework: canonical upstream `master` at `a916ed52e00f364a8615b97cd597858aa7f385dc`
(tree `7293a6e386ffdcf81ac74adc6d26a5505f7319d0`). The shared PRs 485, 486 and 498-501
are merged. The combined framework passed the bounded MMX6, Tomba, Tomba 2
and Ape Escape regression suite plus focused runtime, codegen and real-GL tests.
Historical integration pins below describe earlier work; the gitlink and
`project-manifest.toml` identify the current dependency. This does not expand
the gameplay coverage claims or qualify a release package.

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
