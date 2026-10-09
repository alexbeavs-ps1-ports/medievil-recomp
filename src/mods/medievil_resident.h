#pragma once
#include <stdint.h>

/* Original USA engine resource interface. Native callers allocate first.
 * Request(resource_id) reads padded MWD bytes into info+0x10; Poll(resource_id)
 * returns zero when complete and clears gp+0x84c. Allocation, resource decoding,
 * postprocessing, GPU/SPU transfers and release remain original game routines.
 * There is one in-flight resource, as in the native MR resource service.
 * Expansion1 record is snapshot-backed; never store host pointers in snapshots.
 */
namespace medievil_resident {
constexpr const char* Plugin = "medievil.resident";
constexpr uint32_t Request = 0x800A8200u, Poll = 0x800A8338u;
constexpr uint32_t ArchiveLba = 0x800F41F0u, ResourceTable = 0x800F41F8u;
constexpr uint32_t Error = 0x83Cu, Loading = 0x84Cu, DiscMode = 0x850u;
constexpr uint32_t Observing = 0x4D455631u, Ready = 0x4D455632u;
constexpr uint32_t RecordBytes = 32u;
/* +0 state,+4 id,+8 destination,+12 padded bytes,+16 lba,+20 gp,+24 info,+28 file. */
}
