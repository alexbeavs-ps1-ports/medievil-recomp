#include "mod_plugins.h"
#include "medievil_resident.h"
PSX_MOD_CONSTRUCTOR(register_medievil_resident_reference) {
    psx_mod_register_activation_plugin(medievil_resident::Plugin, [] {
        psx_mod_counter_add("medievil.resident.reference", 1);
    });
}
