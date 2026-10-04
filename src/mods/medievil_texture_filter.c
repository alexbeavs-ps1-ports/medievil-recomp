#include "mod_plugins.h"
static void nearest(void) { psx_mod_set_texture_filter(0); }
static void bilinear(void) { psx_mod_set_texture_filter(1); }
static void stable(void) { psx_mod_set_texture_filter(2); }
PSX_MOD_CONSTRUCTOR(register_texture_filter) {
    psx_mod_register_activation_plugin("medievil.filter.nearest",nearest);
    psx_mod_register_activation_plugin("medievil.filter.bilinear",bilinear);
    psx_mod_register_activation_plugin("medievil.filter.stable",stable);
}
