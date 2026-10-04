#include "mod_plugins.h"

/* Presentation follows completed game images; simulation, input, timers and
 * audio keep their original cadence. Zero follows the current display. */
static void set_rate(unsigned rate) {
    (void)psx_mod_set_frame_interpolation_source(PSX_MOD_FRAME_SOURCE_FLIP);
    (void)psx_mod_set_frame_interpolation_blend(
        PSX_MOD_FRAME_INTERPOLATION_MOTION_ADAPTIVE);
    (void)psx_mod_set_frame_interpolation(rate);
}

#define RATE_CALLBACK(name, rate) \
    static void activate_##name(void) { set_rate(rate); }
RATE_CALLBACK(display, 0)
RATE_CALLBACK(60, 60)
RATE_CALLBACK(120, 120)
RATE_CALLBACK(144, 144)
RATE_CALLBACK(240, 240)
RATE_CALLBACK(360, 360)

PSX_MOD_CONSTRUCTOR(medievil_register_frame_rate) {
#define REGISTER_RATE(name) \
    (void)psx_mod_register_activation_plugin( \
        "medievil.framerate." #name, activate_##name)
    REGISTER_RATE(display);
    REGISTER_RATE(60);
    REGISTER_RATE(120);
    REGISTER_RATE(144);
    REGISTER_RATE(240);
    REGISTER_RATE(360);
}
