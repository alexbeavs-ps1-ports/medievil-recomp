#include "medievil_resident.h"
#include "cpu_state.h"
#include "mod_plugins.h"
#include "mod_resident.h"
#include <cstdlib>
#include <initializer_list>

namespace medievil_resident {
namespace {
#include "medievil_resident_catalog.inc"
const PSXResidentPack* pack = nullptr;
uint32_t record = 0;
bool observe = false, nested = false;
uint32_t rd(uint32_t address) { return psx_mod_read_word(address); }
void wr(uint32_t address, uint32_t value) { psx_mod_write_word(address, value); }
bool ram(uint32_t address, uint32_t bytes) {
    const uint32_t phys = address & 0x1FFFFFFFu;
    return address >= 0x80000000u && address < 0xA0000000u &&
           phys >= 0x10000u && phys < 0x200000u && bytes <= 0x200000u - phys;
}
void finish(CPUState* cpu, uint32_t result) {
    cpu->gpr[2] = result;
    psx_mod_finish_function(cpu);
}
void request(CPUState* cpu) {
    wr(record, 0);
    const uint32_t id = cpu->gpr[4], gp = cpu->gpr[28], ra = cpu->gpr[31];
    if ((ra != 0x800A8080u && ra != 0x800A8114u) || id >= 4096u ||
        !ram(gp, DiscMode + 4u) || rd(gp + DiscMode) != 1u) return;
    const uint32_t table = rd(ResourceTable);
    if (!ram(table, (id + 1u) * 32u)) return;
    const uint32_t info = table + id * 32u;
    const uint32_t destination = rd(info + 16u), size = rd(info + 24u);
    if (!size || size > 0x200000u) return;
    const uint32_t bytes = (size + 2047u) & ~2047u;
    if (!ram(destination, bytes)) return;
    const uint64_t lba64 = uint64_t(rd(ArchiveLba)) + rd(info + 12u);
    if (lba64 > UINT32_MAX) return;
    const uint32_t lba = uint32_t(lba64);
    uint32_t file = UINT32_MAX;
    const uint8_t* data = psx_resident_find_lba(pack, lba, bytes, &file);
    if (!data || !psx_resident_file_stock(pack, file) ||
        rd(ArchiveLba) != psx_resident_file_lba(pack, file)) {
        psx_mod_counter_add("medievil.resident.native_fallback", 1);
        return;
    }
    if (!psx_resident_guest_ranges_match(kContract, 1u, kContractSha256)) {
        psx_mod_counter_add("medievil.resident.contract_mismatch", 1);
        return;
    }
    wr(record + 4, id); wr(record + 8, destination); wr(record + 12, bytes);
    wr(record + 16, lba); wr(record + 20, gp); wr(record + 24, info); wr(record + 28, file);
    psx_resident_record("medievil.resident.begin", file, lba, bytes, 0);
    if (observe) {
        wr(record, Observing);
        psx_mod_counter_add("medievil.resident.assets_observed", 1);
        return; // Original request, CD callbacks and polling run.
    }
    if (!psx_mod_dma_write_ram(destination, data, bytes, int(lba))) return;
    wr(gp + Error, 0);
    wr(record, Ready);
    psx_mod_counter_add("medievil.resident.assets_served", 1);
    psx_mod_counter_add("medievil.resident.bytes_served", bytes);
    // Callers do not consume the request's return; success matches CdRead.
    finish(cpu, 1);
}
void poll(CPUState* cpu) {
    const uint32_t state = rd(record), id = cpu->gpr[4], gp = cpu->gpr[28];
    if ((state != Observing && state != Ready) || rd(record + 4) != id || rd(record + 20) != gp)
        return;
    const uint32_t info = rd(record + 24), destination = rd(record + 8), bytes = rd(record + 12);
    if (!ram(info, 32u) || rd(info + 16) != destination ||
        ((uint64_t(rd(info + 24)) + 2047u) & ~uint64_t(2047u)) != bytes) {
        // A changed caller layout uses the current native request/poll pair.
        wr(record, 0);
        if (state == Ready) {
            nested = true;
            psx_mod_call_guest(cpu, Request, cpu->gpr[31], id, cpu->gpr[5], cpu->gpr[6], cpu->gpr[7]);
            nested = false;
        }
        psx_mod_counter_add("medievil.resident.native_fallback", 1);
        return;
    }
    if (state == Observing) {
        nested = true;
        const uint32_t result = psx_mod_call_guest(cpu, Poll, cpu->gpr[31],
            id, cpu->gpr[5], cpu->gpr[6], cpu->gpr[7]);
        nested = false;
        if (result == 0) {
            const uint8_t* data = psx_resident_find_lba(pack, rd(record + 16), bytes, nullptr);
            bool match = data && rd(gp + Error) == 0 && rd(gp + Loading) == 0;
            for (uint32_t i = 0; match && i < bytes; i += 4u)
                match = rd(destination + i) == (uint32_t(data[i]) | uint32_t(data[i+1]) << 8 |
                    uint32_t(data[i+2]) << 16 | uint32_t(data[i+3]) << 24);
            psx_mod_counter_add(match ? "medievil.resident.native_match" : "medievil.resident.native_mismatch", 1);
            psx_mod_counter_add("medievil.resident.native_bytes_compared", bytes);
            psx_resident_record("medievil.resident.end", rd(record + 28), rd(record + 16), bytes, 0);
            wr(record, 0);
        } else if (result == UINT32_MAX) wr(record, 0);
        finish(cpu, result);
        return;
    }
    wr(gp + Loading, 0);
    psx_resident_record("medievil.resident.end", rd(record + 28), rd(record + 16), bytes, 1);
    wr(record, 0);
    psx_mod_counter_add("medievil.resident.polls_served", 1);
    finish(cpu, 0);
}
void entry(CPUState* cpu, uint32_t address) {
    if (!pack || !record || nested) return;
    address |= 0x80000000u;
    if (address == Request && rd(address) == 0x27BDFFD8u) request(cpu);
    else if (address == Poll && rd(address) == 0x27BDFFE8u) poll(cpu);
}
void activate() {
    pack = nullptr; record = 0; nested = false;
    const char* diagnostic = std::getenv("PSX_MEDIEVIL_RESIDENT_OBSERVE");
    observe = diagnostic && diagnostic[0] == '1';
    PSXResidentSpec spec{};
    spec.struct_size = sizeof spec; spec.title = "MediEvil_Recompiled";
    spec.format = "scus94227-mwd-v1"; spec.files = kFiles; spec.file_count = 1u;
    spec.policy = PSX_RESIDENT_ALLOW_MODIFIED;
    pack = psx_resident_prepare(&spec);
    if (pack) record = psx_mod_alloc_guest_memory(RecordBytes, 4);
    if (!record) { pack = nullptr; psx_mod_counter_add("medievil.resident.unavailable", 1); }
    else psx_mod_counter_add("medievil.resident.activated", 1);
}
}
}
PSX_MOD_CONSTRUCTOR(register_medievil_resident) {
    using namespace medievil_resident;
    psx_mod_register_activation_plugin(Plugin, activate);
    for (uint32_t address : {Request, Poll})
        psx_mod_register_function_entry_plugin(Plugin, address, entry);
}
