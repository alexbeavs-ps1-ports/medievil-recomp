#include "mod_plugins.h"
#include "cpu_state.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define PKG "medievil.enhancement.widescreen"
#define PLUGIN "medievil.widescreen"

/* The guarded disc patches relocate every reference to the marked-cell list
 * here. These bounded render-only arenas occupy otherwise unused expanded RAM;
 * game heap, object lifetimes, fog/depth and terrain subdivision stay stock. */
enum {
    CELL_LIST = 0x80300000u, CAPTURE_LIST = 0x80302000u,
    POLY_LIST = 0x80310000u, PRIM0 = 0x80400000u,
    PRIM1 = 0x80500000u, PRIM_BYTES = 0x100000u,
    CAPTURE_CAP = 1024u, PRIM_CAP = 8192u,
    TERRAIN = 0x800EEDC0u, CORNERS = 0x800EEA04u
};

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

static void capture(CPUState* cpu, uint32_t address) {
    if (psx_mod_read_word(address) != 0x27BDFF08u ||
        psx_mod_read_word(address + 4) != 0xAFB500E4u) return;
    configure_arenas();
    cpu->gpr[5] = CAPTURE_CAP;
    cpu->gpr[6] = CAPTURE_LIST;
    int margin = psx_mod_widescreen_x_margin();
    unsigned width = psx_mod_display_width();
    if (!width) width = 512;
    /* The view-plane SVECs use a 320-unit horizontal span, independently of
     * the GPU's 512-pixel 3D display. Round outwards and include the cull guard. */
    int half = 160 + (margin > 0 ? (int)(((uint64_t)margin * 320u + width - 1u) / width) : 0);
    if (half > 32767) half = 32767;
    /* Change only camera-horizontal frustum corners. Y/Z remain game-owned. */
    for (unsigned i = 0; i < 4; ++i) {
        int x = i < 2 ? -half : half;
        psx_mod_write_half(CORNERS + i * 8, (uint16_t)(int16_t)x);
    }
}

static void render(CPUState* cpu, uint32_t address) {
    (void)cpu;
    if (psx_mod_read_word(address) == 0x27BDFF50u)
        configure_arenas();
}

static void activate(void) {
    char view[16];
    if (!psx_mod_set_main_ram_8mb(1)) {
        fprintf(stderr, "MediEvil: expanded render memory unavailable\n");
        abort();
    }
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
    (void)psx_mod_register_function_entry_plugin(PLUGIN, 0x8005176Cu, capture);
    (void)psx_mod_register_function_entry_plugin(PLUGIN, 0x80021CECu, render);
}
