#include "mod_plugins.h"
#include "cpu_state.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define PKG "medievil.enhancement.widescreen"
#define PLUGIN "medievil.widescreen"

/* Guest-owned capture records and cleanup sentinel retain their stock layout.
 * Only render data occupy expanded RAM; the original heap allocation remains
 * in TERRAIN+0 so the guest's terrain teardown still frees its own memory. */
enum {
    CELL_LIST = 0x80300000u, CAPTURE_LIST = 0x80304000u,
    FOG_BUFFER = 0x80308000u, FOG = 0x800F17B8u,
    POLY_LIST = 0x80310000u, PRIM0 = 0x80400000u,
    PRIM1 = 0x80500000u, PRIM_BYTES = 0x100000u,
    CAPTURE_CAP = 2048u, PRIM_CAP = 8192u,
    GRID_MAX = 129u, FOG_CAP = 1024u, META = 0x80308C00u,
    TERRAIN = 0x800EEDC0u, CORNERS = 0x800EEA04u
};

static unsigned distance_scale = 3, bypass_subdivision = 1;

static int ram(uint32_t p, uint32_t bytes) {
    return p >= 0x80010000u && p < 0x80200000u &&
           bytes <= 0x80200000u - p;
}

/* The engine maps SZ to an OT bucket by a right shift. Raising that shift
 * extends its representable reach without reallocating guest-owned OTs.
 * All paths still see the same table size and linked-list terminators. */
static int fog_init(CPUState* cpu, uint32_t address) {
    if (psx_mod_read_word(address) != 0x27BDFFE0u ||
        psx_mod_read_word(address + 4) != 0xAFB00010u) return 0;
    uint32_t vp = psx_mod_read_word(cpu->gpr[28] + 0x5E0);
    unsigned reach = cpu->gpr[4] & 0xFFFFu;
    if (!ram(vp, 0x12C) || !reach || reach > 32768u) return 0;
    unsigned original_reach = reach;
    unsigned wanted = reach * distance_scale, delta = 0;
    while (reach < wanted && reach < 32768u) { reach *= 2; ++delta; }
    if (reach > 32768u) reach = 32768u;
    unsigned shift = psx_mod_read_half(vp + 0x6C);
    if (shift + delta > 15) return 0;
    psx_mod_write_half(vp + 0x6E, (uint16_t)reach);
    psx_mod_write_half(vp + 0x6C, (uint16_t)(shift + delta));
    psx_mod_write_word(META, vp);
    psx_mod_write_word(META + 4, original_reach);
    psx_mod_write_word(META + 8, shift);
    psx_mod_write_word(META + 12, reach);
    psx_mod_write_word(META + 16, shift + delta);
    for (unsigned i = 0; i < 40; i += 4) psx_mod_write_word(FOG + i, 0);
    psx_mod_write_word(FOG, FOG_BUFFER);
    psx_mod_write_half(FOG + 4, (uint16_t)(reach >> 5));
    for (unsigned i = 0; i < FOG_CAP + 64; ++i)
        psx_mod_write_half(FOG_BUFFER + i * 2, 0xFFF);
    psx_mod_write_word(META + 20, 0);
    psx_mod_write_word(META + 24, 0);
    cpu->gpr[2] = 0;
    fprintf(stdout, "MediEvil terrain: distance=%ux, OT reach=%u, shift=%u, subdivision=%s\n",
            distance_scale, reach, shift + delta, bypass_subdivision ? "bypassed" : "original");
    return 1;
}

static int fog_free(CPUState* cpu, uint32_t address) {
    if (psx_mod_read_word(address) != 0x27BDFFE8u ||
        psx_mod_read_word(address + 4) != 0xAFB00010u ||
        psx_mod_read_word(FOG) != FOG_BUFFER) return 0;
    /* This buffer was never allocated from the guest heap. */
    uint32_t vp = psx_mod_read_word(META);
    if (ram(vp, 0x12C) && psx_mod_read_half(vp + 0x6E) == psx_mod_read_word(META + 12) &&
        psx_mod_read_half(vp + 0x6C) == psx_mod_read_word(META + 16)) {
        psx_mod_write_half(vp + 0x6E, (uint16_t)psx_mod_read_word(META + 4));
        psx_mod_write_half(vp + 0x6C, (uint16_t)psx_mod_read_word(META + 8));
    }
    psx_mod_write_word(FOG, 0);
    psx_mod_write_word(META + 20, 0);
    psx_mod_write_word(META + 24, 0);
    cpu->gpr[2] = 0;
    return 1;
}

static void distance(CPUState* cpu, uint32_t address) {
    (void)cpu;
    if (psx_mod_read_word(address) != 0x3C02800Fu ||
        psx_mod_read_word(address + 4) != 0x944217BEu) return;
    unsigned current = psx_mod_read_half(TERRAIN + 0x24);
    if (!current || current >= 32768u || psx_mod_read_word(FOG) != FOG_BUFFER) return;
    /* Accept engine changes (including a newly loaded level), never multiply
     * our previously scaled value again. The stock update rebuilds fog and
     * BackOfOT when +24 differs from +26, before it calls terrain capture. */
    unsigned base_distance = psx_mod_read_word(META + 20);
    unsigned applied_distance = psx_mod_read_word(META + 24);
    if (!base_distance || current != applied_distance) base_distance = current;
    unsigned limit = psx_mod_read_half(FOG + 4) << 5;
    unsigned scaled = base_distance * distance_scale;
    if (scaled >= limit) scaled = limit - 1;
    if (scaled > 32767u) scaled = 32767u;
    if (scaled < base_distance) scaled = base_distance;
    psx_mod_write_word(META + 20, base_distance);
    psx_mod_write_word(META + 24, scaled);
    psx_mod_write_half(TERRAIN + 0x24, (uint16_t)scaled);
}

typedef struct Cell { uint32_t address; uint64_t distance; } Cell;
static Cell cells[GRID_MAX * GRID_MAX];

static int nearer(const void* a, const void* b) {
    const Cell *x = a, *y = b;
    if (x->distance != y->distance) return x->distance < y->distance ? -1 : 1;
    return x->address < y->address ? -1 : x->address != y->address;
}

static void configure_arenas(void) {
    psx_mod_write_word(TERRAIN + 4, PRIM0);
    psx_mod_write_word(TERRAIN + 8, PRIM1);
    psx_mod_write_word(TERRAIN + 12, CAPTURE_LIST);
    psx_mod_write_word(TERRAIN + 16, POLY_LIST);
    psx_mod_write_word(TERRAIN + 20, PRIM_CAP);
    /* The guest renderer additionally reserves 0xD0 bytes before emitting. */
    psx_mod_write_word(TERRAIN + 24, PRIM0 + PRIM_BYTES - 256u);
    psx_mod_write_word(TERRAIN + 28, PRIM1 + PRIM_BYTES - 256u);
}

static int capture(CPUState* cpu, uint32_t address) {
    if (psx_mod_read_word(address) != 0x27BDFF08u ||
        psx_mod_read_word(address + 4) != 0xAFB500E4u) return 0;
    configure_arenas();
    cpu->gpr[5] = CAPTURE_CAP;
    cpu->gpr[6] = CAPTURE_LIST;
    int margin = psx_mod_widescreen_x_margin();
    unsigned width = psx_mod_display_width();
    if (!width) width = 512;
    /* The view-plane SVECs use a 320-unit horizontal span, independently of
     * the GPU's 512-pixel 3D display. Round outwards and include the cull guard. */
    int half = 160 + (margin > 0 ? (int)(((uint64_t)margin * 320u + width - 1u) / width) : 0);
    if (half > 2048) half = 2048;
    /* Conservative overscan for tilted cameras and tall walls; preserve the
     * game-controlled focal distance in Z. These vectors are capture-only. */
    half = (half * 5 + 3) / 4;
    for (unsigned i = 0; i < 4; ++i) {
        int x = i < 2 ? -half : half;
        psx_mod_write_half(CORNERS + i * 8, (uint16_t)(int16_t)x);
        psx_mod_write_half(CORNERS + i * 8 + 2, (uint16_t)(int16_t)(i == 0 || i == 3 ? 192 : -192));
    }

    uint32_t gp = cpu->gpr[28], camera = cpu->gpr[4];
    uint32_t map = psx_mod_read_word(0x800EEE7Cu);
    if (!ram(camera, 0x80) || !ram(map, 0x70)) return 0;
    uint32_t grid = psx_mod_read_word(map + 0x6C);
    if (!ram(grid, 16)) return 0;
    unsigned nx = psx_mod_read_byte(grid), nz = psx_mod_read_byte(grid + 1);
    unsigned shift = psx_mod_read_word(gp + 0x5AC);
    unsigned len = psx_mod_read_word(gp + 0x5BC);
    uint32_t ids = psx_mod_read_word(grid + 8), data = psx_mod_read_word(grid + 12);
    if (!nx || !nz || nx > GRID_MAX || nz > GRID_MAX || shift < 1 || shift > 15 ||
        len != (1u << shift) || nx != psx_mod_read_word(gp + 0x5C0) ||
        nz != psx_mod_read_word(gp + 0x5C8) || !ram(ids, nx * nz * 2) || !ram(data, 8)) return 0;

    int plane_z = (int16_t)psx_mod_read_half(CORNERS + 4);
    unsigned far = psx_mod_read_half(TERRAIN + 0x24);
    if (plane_z <= 0 || !far || far >= 32768) return 0;
    /* A ground-intersection frustum can discard a tall wall while its upper
     * edge remains visible. Select a conservative XZ disk instead: every
     * cell intersecting the far-plane footprint is a candidate, regardless
     * of camera tilt or the player/ground height. The original polygon funnel
     * still rejects backfaces, near/depth failures and offscreen primitives. */
    double radius = far * sqrt((double)half * half + 192.0 * 192 +
                              (double)plane_z * plane_z) / plane_z + len * 1.5;
    uint64_t radius2 = (uint64_t)(radius * radius);
    int32_t cam_x = (int32_t)psx_mod_read_word(camera + 0x74);
    int32_t cam_z = (int32_t)psx_mod_read_word(camera + 0x7C);
    int base_x = (int16_t)psx_mod_read_half(gp + 0x5B4);
    int base_z = (int16_t)psx_mod_read_half(gp + 0x5B8);
    unsigned found = 0;
    for (unsigned z = 0; z < nz; ++z) for (unsigned x = 0; x < nx; ++x) {
        int id = (int16_t)psx_mod_read_half(ids + (z * nx + x) * 2);
        if (id < 0) continue;
        uint32_t cell = data + (unsigned)id * 8;
        if (!ram(cell, 8)) continue;
        unsigned polys = psx_mod_read_half(cell) & 0x7FFF;
        if (!polys || !ram(psx_mod_read_word(cell + 4), polys * 2)) continue;
        int64_t dx = (int64_t)base_x + (x << shift) + len / 2 - cam_x;
        int64_t dz = (int64_t)base_z + (z << shift) + len / 2 - cam_z;
        uint64_t d2 = (uint64_t)(dx * dx) + (uint64_t)(dz * dz);
        if (d2 > radius2) continue;
        cells[found++] = (Cell){cell, d2};
    }
    qsort(cells, found, sizeof cells[0], nearer);
    unsigned captured = 0, spent = 0, budget_skipped = 0;
    for (unsigned i = 0; i < found && captured < CAPTURE_CAP; ++i) {
        uint32_t cell = cells[i].address;
        unsigned count = psx_mod_read_half(cell);
        if (count & 0x8000) continue; /* multiple grid squares can share a cell */
        if (spent + count > PRIM_CAP) { ++budget_skipped; continue; }
        uint32_t entry = CAPTURE_LIST + captured * 8;
        psx_mod_write_word(entry, count); /* clears record padding as well */
        psx_mod_write_word(entry + 4, psx_mod_read_word(cell + 4));
        psx_mod_write_word(CELL_LIST + captured * 4, cell);
        psx_mod_write_half(cell, (uint16_t)(count | 0x8000));
        spent += count;
        ++captured;
    }
    psx_mod_write_word(CELL_LIST + captured * 4, 0); /* guest cleanup sentinel */
    psx_mod_write_word(META + 32, found);
    psx_mod_write_word(META + 36, captured);
    psx_mod_write_word(META + 40, spent);
    psx_mod_write_word(META + 44, budget_skipped);
    cpu->gpr[2] = captured;
    return 1;
}

static void render(CPUState* cpu, uint32_t address) {
    (void)cpu;
    if (psx_mod_read_word(address) != 0x27BDFF50u) return;
    configure_arenas();
    /* Subdivision is selected by guarded disc patches and independently
     * compiled AOT images. Never invalidate executable RAM every launch. */
}

static void activate(void) {
    distance_scale = 3;
    bypass_subdivision = 1;
    char view[16];
    if (!psx_mod_set_main_ram_8mb(1)) {
        fprintf(stderr, "MediEvil: expanded render memory unavailable\n");
        abort();
    }
    psx_mod_set_native_wide_projection_correction(1);
    const uint32_t cull_sites[] = {0x80021EACu, 0x8002249Cu, 0x800224B0u};
    const uint32_t cull_words[] = {0x1900FFCFu, 0x1D000006u, 0x0501FE4Eu};
    psx_mod_set_native_wide_nclip_sites(cull_sites, cull_words, 3);
    char value[16];
    if (psx_mod_option_value(PKG, "widescreen", "draw_distance", value, sizeof value)) {
        if (!strcmp(value, "1x")) distance_scale = 1;
        else if (!strcmp(value, "2x")) distance_scale = 2;
    }
    if (psx_mod_option_value(PKG, "widescreen", "subdivision_bypass", value, sizeof value))
        bypass_subdivision = !strcmp(value, "true");
    if (!psx_mod_option_value(PKG, "widescreen", "aspect", view, sizeof view))
        strcpy(view, "Fit");
    unsigned numerator = 16;
    if (!strcmp(view, "4:3")) {
        (void)psx_mod_set_fixed_display_aspect(4, 3);
        return;
    }
    if (!strcmp(view, "21:9")) numerator = 21;
    if (!strcmp(view, "32:9")) numerator = 32;
    (void)psx_mod_set_fixed_display_aspect(numerator, 9);
    if (!strcmp(view, "Fit")) (void)psx_mod_set_adaptive_display_aspect(0, 0);
}

PSX_MOD_CONSTRUCTOR(medievil_register_widescreen) {
    (void)psx_mod_register_activation_plugin(PLUGIN, activate);
    (void)psx_mod_register_function_filter_plugin(PLUGIN, 0x8005176Cu, capture);
    (void)psx_mod_register_function_entry_plugin(PLUGIN, 0x80021CECu, render);
    (void)psx_mod_register_function_entry_plugin(PLUGIN, 0x800514FCu, distance);
    (void)psx_mod_register_function_filter_plugin(PLUGIN, 0x8007A02Cu, fog_init);
    (void)psx_mod_register_function_filter_plugin(PLUGIN, 0x8007A0C0u, fog_free);
}
