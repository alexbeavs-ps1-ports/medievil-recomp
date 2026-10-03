# MediEvil validation receipt

The receipt below records the original standup. The 2026-10-02 enhancement
branch's source inputs, AOT audit, Release build and headless startup results are
recorded in [ENHANCEMENTS.md](ENHANCEMENTS.md). Neither receipt establishes full
gameplay quality.

The enhancement branch defaults to bundled OpenBIOS with the BIOS shell skipped.
Its adaptive native-wide view has been checked through Dan's Crypt, including
dialogue, player movement, 16:9/21:9/32:9 and live window resizing. Engine and
level entries used static native dispatch. Wider capture and culling hooks
retain the original vertical/depth tests. The latest build uses the 1080p
preset, PGXP geometry/perspective correction and CPU provenance by default.
Terrain distance defaults to 3x; Original and 2x are selectable, and
subdivision bypass defaults on. Both selections have separately verified native
AOT images. Smooth Presentation defaults on at Display refresh, with fixed
60/120/144/240/360 choices and original simulation timing. PGXP now belongs to
a default-on mod whose session choice survives renderer initialization.
The final default-state Crypt route showed the room beyond the closed gate
from the coffin and gate-side views, with no cells shed by the capture budget.
The gate was not crossed in that route. Display requested 165 Hz and achieved
about 132 presents/sec; steady target throughput remains unqualified. See the
enhancement receipt for the measured timing and option states.
Saturation-aware terrain winding
and conservative tall-wall capture fill the previously observed Crypt gaps.
Audio, save/load and the remaining
levels have not been qualified by that route. Earlier checks also compared
intro/engine behavior against the owned disc and a retail BIOS.
The historical retail-only requirement below does not describe this branch.

## Scope

- Game: MediEvil, USA, `SCUS-94227`
- Version: `0.1.0`
- Catalog ID: `medievil-psx`
- Release repository: `Alexbeav/medievil-recomp`
- Publication state: not published

## Frozen inputs

- The source disc identity is in `catalog_identity.json`.
- The required BIOS is a legal SCPH-1001 dump. The package does not support
  OpenBIOS.
- `framework_pins.txt` and the submodule gitlinks record the framework inputs.
- Generated retail code, the game executable, the disc, and the BIOS remain
  outside Git.

## Required release gates

The release candidate must pass emitter generation, Release build, headless
startup, clean source package, payload, license, and clean-path checks. Alex
must then pass visible gameplay from the exact package. A headless test does not
replace that gameplay test.

## Local runtime evidence

The pinned emitters generated the game and SCPH-1001 backends from the recorded
owned inputs. The portable toolchain built the Release runtime. A hidden,
launcher-free, software-renderer run stayed active for 25 seconds and reached
frame 3,166 with 3,166 VBlank raises and no fatal state.

The watchdog wrote one `spin_freeze` snapshot at frame 1,002. The same process
then continued for 21 seconds and 2,164 frames. `PSX-DIAG-007` classifies this
file as a self-resolved wait-loop snapshot, not a terminal crash. The run still
needs visible gameplay and exact-package tests.
