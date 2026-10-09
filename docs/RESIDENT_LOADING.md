# MediEvil I resource loading

Tracking: `beads-eio.18.14`. Original USA engine `MEDIEVIL.EXE`, game
`SCUS-94227`. The candidate caches `MEDRES.MWD` from the owner's effective disc
using the shared psxrecomp resident store. The original archive is 25,362,432
bytes. First launch prepares a local pack; later launches verify/reuse it.
Only metadata and SHA-256 guards are committed; no asset bytes are shipped.
Movies, XA and CD music are not included in this pack.

## Caller contract

The native resource table is read through `800F41F8`, with 32-byte records.
Archive LBA is at `800F41F0`. A record's `+0C` is its relative sector offset,
`+10` its allocated destination, and `+18` its stored byte size. The native
allocator handles normal and compressed resources before starting a request.

| Boundary | Supported interface and completion |
| --- | --- |
| Request `800A8200` | Resource ID in `a0`, called from the native synchronous (`ra=800A8080`) or asynchronous (`ra=800A8114`) wrapper, CD mode `gp+850=1`. Publish sector-padded archive bytes into the native destination and clear the request error at `gp+83C`. |
| Poll `800A8338` | Same resource ID, table and destination. Once data is ready, return `v0=0` and clear loading at `gp+84C`. The asynchronous level-state caller and synchronous wrapper remain native. |

Allocation `800A8154`, processing/decoding `800A83E0`, GPU/SPU transfers and
release `800A8564` remain original routines. There is one pending resource,
matching the original service. Its 32-byte adapter record is in snapshot-backed
enhancement memory, rather than unsaved host state. The stock executable's
`800A804C..800A8684` code range and stock archive SHA constrain activation.

The adapter declines unsupported callers, invalid extents and changed archive
content. It uses the effective archive LBA, so disc relocation is respected.
A changed in-flight destination restarts the original request and retains its
native polling path. Shared DMA publication preserves memory invalidation and
capture effects. It adds no CD-speed multiplier or generic host pacing.

## Build selection and validation

Separate output directories with `PSX_EXECUTION_PROFILE=ENHANCED` and
`PSX_EXECUTION_PROFILE=REFERENCE` select the resident and original implementations
at build time. REFERENCE registers no resource hooks or resident pack. The
ordinary Resident Loading product feature defaults on; it is not a runtime
implementation-profile selector. Keep source-generated AOT and execution
contracts beside the selected build; cross-build savestates are unsupported.

`PSX_MEDIEVIL_RESIDENT_OBSERVE=1` is a private integration diagnostic. It runs
the actual original request and poll, then compares all padded destination
bytes and the error/loading completion fields before native decoding starts.
It neither serves cached data nor exposes a product implementation switch.

The stock archive/code guard generator reuses shared `resident_catalog.py`:

```sh
python tools/medievil_resident_catalog.py --disc "disc/MediEvil (USA).cue" --framework psxrecomp --output src/mods/medievil_resident_catalog.inc --check
```

The Windows ENHANCED candidate compiles with bundled OpenBIOS and guarded
original-disc static AOT. A bounded menu/New Game check reached Dan's Crypt
with runtime overlay compilation disabled. The native diagnostic completed
17 resource reads: all 2,101,248 padded destination bytes and the error/loading
fields matched. The enhanced run served and completed those same 17 reads;
both checks exited cleanly.

Matched request-to-poll windows totalled 509 guest frames on the original path
and zero on the enhanced path. This measures the resource reads, not whole
launch time, decoding, presentation waits or steady gameplay performance.
The enhanced binary SHA-256 is
`f3fcba2c2c0805bb585c697f97e2410e955d725eeb19e664def11db59f4b63cb`;
its execution identity is
`b8e42e59883ae679fb0e83c54d482fb1088668d8514bd76bdaf1381755d0457a`.

The separate REFERENCE build also reached the Crypt and exited cleanly,
with only `medievil.resident.reference=1` and no resident reads served. It
retains the original loader. On 2026-10-08 the owner approved the resident
loading review candidate and requested that it be committed and the serial
queue continued. The implementation is `e3fd449` on
`feat/medievil1-parity-hle-20261006`; no release was requested.
Hidden checks disable audio/controllers. This bounded qualification and owner
acceptance do not establish every-level or resource-mod coverage, or
cross-build savestate compatibility.
