# MediEvil I serial review

Tracking: `beads-eio.18.13`. Worktree: `feat/medievil1-parity-hle-20261006`.
The current public title main is an ancestor of this branch. Framework and UI
pins are recorded in `framework_pins.txt`.

The review preset uses bundled OpenBIOS, OpenGL at 1080p, PGXP geometry and
perspective textures, and stable world filtering. Native scene interpolation
remains available, **default off**, per the owner's exception for MediEvil.
Generic CD Speed and Fast Loading (host pacing) are excluded.

The former combined view control is split into three default-on features with
independent plugin identities:

| Control | Plugin | Behavior |
| --- | --- | --- |
| Adaptive View | `medievil.widescreen` | Fit to Window or fixed aspect; expanded terrain capture and precise projection/clipping. |
| Extended Draw Distance | `medievil.draw-distance` | 2x or 3x terrain/fog reach, default 3x; guarded fog allocation and cleanup. |
| Terrain Subdivision Fix | `medievil.terrain-subdivision` | Two original-disc guarded patches bypass the stock subdivision threshold, with perspective-correct rendering. |

Disabling one feature no longer removes the other features' plugin hooks.
Subdivision selection uses original-disc patches and compiled image variants;
the plugin never rewrites executable RAM. Draw distance owns its fog lifetime
independently of wider terrain capture. Capture buffers and primitive budgets
remain bounded in expanded memory.

Configure a separate build with `PSX_EXECUTION_PROFILE=REFERENCE` to retain
the original execution implementations. `ENHANCED` is the review default.
Neither profile changes original game simulation through optional draw replay.
Interpolation qualification is outside this control split.

The owned USA disc contains `MEDIEVIL.EXE` and 26 uncompressed overlay images.
The declarative AOT profile includes 29 original-disc recipes with view and
subdivision image variants. Static C is compiled into the PGXP runtime with
its target definitions; it has no DLL flavor or cache namespace. Original byte
guards and generated-file receipts remain required. This inventory does not
prove full native coverage of every level.

There is no MediEvil I asset-loader adapter yet. The code overlays are already
uncompressed, so MediEvil II's PP20 decoder is not applicable. Title-specific
asset loading and broader residency remain open; no loading-speed improvement
is claimed. The headless Ghidra registry timed out during this bounded
inspection. The pinned decomp source was retrieved using the profile's exact
commit; its engine file is an inventory of unnamed assembly functions, without
an identified asset-loader contract.

Light validation covers real terrain/fog record contracts, exact manifest
bindings and guarded AOT generation, compilation, and a short OpenBIOS startup.
Human review checks gameplay, audio, transitions and the three controls:

1. Start New Game and reach Dan's Crypt.
2. Rotate the camera near walls and the gate; check world edges and textures.
3. Disable and re-enable each view feature independently; compare terrain
   reach and subdivision with interpolation left off; restart if the UI asks.
4. Leave the Crypt, check HUD and dialogue, and save/load if convenient.

No release, Linux package, all-level acceptance or measured loading gain is
claimed by this preparation.
