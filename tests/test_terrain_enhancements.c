/* Engine-contract fixture: real USA record layouts, bounded enhanced arenas,
 * original cleanup sentinel, fog lifetime and no runtime executable mutation. */
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "cpu_state.h"
#include "mod_plugins.h"

static unsigned char memory[8 * 1024 * 1024];
static unsigned code_writes;
uint8_t psx_mod_read_byte(uint32_t p) { return memory[p & 0x7FFFFF]; }
uint16_t psx_mod_read_half(uint32_t p) { uint16_t v; memcpy(&v, memory + (p & 0x7FFFFF), 2); return v; }
uint32_t psx_mod_read_word(uint32_t p) { uint32_t v; memcpy(&v, memory + (p & 0x7FFFFF), 4); return v; }
void psx_mod_write_byte(uint32_t p, uint8_t v) { memory[p & 0x7FFFFF] = v; }
void psx_mod_write_half(uint32_t p, uint16_t v) { memcpy(memory + (p & 0x7FFFFF), &v, 2); }
void psx_mod_write_word(uint32_t p, uint32_t v) { memcpy(memory + (p & 0x7FFFFF), &v, 4); }
void psx_mod_write_code_word(uint32_t p, uint32_t v) { ++code_writes; psx_mod_write_word(p, v); }
int32_t psx_mod_widescreen_x_margin(void) { return 578; }
uint32_t psx_mod_display_width(void) { return 512; }
int psx_mod_set_main_ram_8mb(int v) { return v; }
void psx_mod_set_native_wide_projection_correction(int v) { (void)v; }
void psx_mod_set_native_wide_nclip_sites(const uint32_t*a,const uint32_t*b,int n) { (void)a;(void)b;(void)n; }
int psx_mod_option_value(const char*a,const char*b,const char*c,char*d,uint32_t n) { (void)a;(void)b;(void)c;(void)n;*d=0;return 0; }
int psx_mod_set_fixed_display_aspect(uint32_t n,uint32_t d) { (void)n;(void)d;return 1; }
int psx_mod_set_adaptive_display_aspect(uint32_t n,uint32_t d) { (void)n;(void)d;return 1; }
int psx_mod_register_activation_plugin(const char*i,PSXModActivationCallback c) { (void)i;(void)c;return 1; }
int psx_mod_register_function_entry_plugin(const char*i,uint32_t a,PSXModFunctionEntryCallback c) { (void)i;(void)a;(void)c;return 1; }
int psx_mod_register_function_filter_plugin(const char*i,uint32_t a,PSXModFunctionFilterCallback c) { (void)i;(void)a;(void)c;return 1; }

#include "../src/mods/medievil_widescreen.c"

static CPUState setup(void) {
    memset(memory, 0, sizeof memory);
    CPUState cpu = {0}; cpu.gpr[28] = 0x800ED5D4; cpu.gpr[4] = 8192;
    psx_mod_write_word(0x8007A02C, 0x27BDFFE0); psx_mod_write_word(0x8007A030, 0xAFB00010);
    psx_mod_write_word(0x8007A0C0, 0x27BDFFE8); psx_mod_write_word(0x8007A0C4, 0xAFB00010);
    psx_mod_write_word(0x800514FC, 0x3C02800F); psx_mod_write_word(0x80051500, 0x944217BE);
    psx_mod_write_word(0x8005176C, 0x27BDFF08); psx_mod_write_word(0x80051770, 0xAFB500E4);
    psx_mod_write_word(cpu.gpr[28]+0x5E0, 0x80110000);
    psx_mod_write_half(0x8011006C, 4);
    psx_mod_write_half(TERRAIN+0x24, 4096);
    distance_scale = 2; bypass_subdivision = 0;
    return cpu;
}

int main(void) {
    CPUState cpu = setup();
    assert(fog_init(&cpu, 0x8007A02C));
    assert(psx_mod_read_half(0x8011006C) == 5);
    assert(psx_mod_read_half(0x8011006E) == 16384);
    assert(psx_mod_read_half(FOG+4) == 512);
    assert(psx_mod_read_half(FOG_BUFFER+1024) == 4095);
    distance(&cpu, 0x800514FC);
    assert(psx_mod_read_half(TERRAIN+0x24) == 8192);
    distance(&cpu, 0x800514FC); /* repeated frames must not compound */
    assert(psx_mod_read_half(TERRAIN+0x24) == 8192);
    psx_mod_write_half(TERRAIN+0x24, 3000); /* engine changes its authored limit */
    distance(&cpu, 0x800514FC);
    assert(psx_mod_read_half(TERRAIN+0x24) == 6000);
    cpu.gpr[4] = 0x80101000;
    psx_mod_write_word(0x800EEE7C, 0x80102000);
    psx_mod_write_word(0x8010206C, 0x80103000);
    psx_mod_write_byte(0x80103000, 3); psx_mod_write_byte(0x80103001, 1);
    psx_mod_write_word(0x80103008, 0x80104000); psx_mod_write_word(0x8010300C, 0x80105000);
    psx_mod_write_word(cpu.gpr[28]+0x5AC, 8); psx_mod_write_word(cpu.gpr[28]+0x5BC, 256);
    psx_mod_write_word(cpu.gpr[28]+0x5C0, 3); psx_mod_write_word(cpu.gpr[28]+0x5C8, 1);
    psx_mod_write_half(0x80104000, 0); psx_mod_write_half(0x80104002, 1); psx_mod_write_half(0x80104004, 0);
    for (unsigned i=0;i<2;i++) {
        psx_mod_write_half(0x80105000+i*8, 1);
        psx_mod_write_half(0x80105002+i*8, 20000); /* tall wall, independent of ground */
        psx_mod_write_word(0x80105004+i*8, 0x80106000+i*2);
    }
    psx_mod_write_half(CORNERS+4, 350);
    psx_mod_write_word(TERRAIN, 0x80107000); /* original heap owner */
    assert(capture(&cpu, 0x8005176C)); assert(cpu.gpr[2] == 2);
    assert(psx_mod_read_word(CELL_LIST+8) == 0);
    assert(psx_mod_read_half(0x80105000) == 0x8001);
    assert(psx_mod_read_word(CAPTURE_LIST+4) == 0x80106000);
    assert(psx_mod_read_word(TERRAIN) == 0x80107000);
    /* Original cleanup walks addresses until the zero sentinel. */
    for (unsigned i=0;psx_mod_read_word(CELL_LIST+i*4);i++) {
        uint32_t p=psx_mod_read_word(CELL_LIST+i*4);
        psx_mod_write_half(p, psx_mod_read_half(p)&0x7FFF);
    }
    assert(capture(&cpu, 0x8005176C)); assert(cpu.gpr[2] == 2);
    psx_mod_write_byte(0x80103000, 130);
    assert(!capture(&cpu, 0x8005176C)); /* unrecognized layout defers to guest */
    /* Dense grid: prioritize nearby cells and stay inside both capacities. */
    psx_mod_write_byte(0x80103000, 129); psx_mod_write_byte(0x80103001, 129);
    psx_mod_write_word(cpu.gpr[28]+0x5C0, 129); psx_mod_write_word(cpu.gpr[28]+0x5C8, 129);
    psx_mod_write_word(0x80103008, 0x80130000); psx_mod_write_word(0x8010300C, 0x80140000);
    psx_mod_write_word(cpu.gpr[4]+0x74, 16384); psx_mod_write_word(cpu.gpr[4]+0x7C, 16384);
    psx_mod_write_half(TERRAIN+0x24, 32767);
    for (unsigned i=0;i<129*129;i++) {
        psx_mod_write_half(0x80130000+i*2, (uint16_t)i);
        psx_mod_write_half(0x80140000+i*8, 100);
        psx_mod_write_word(0x80140004+i*8, 0x80180000);
    }
    psx_mod_write_word(CAPTURE_LIST+CAPTURE_CAP*8, 0xABCDEF12);
    assert(capture(&cpu, 0x8005176C)); assert(cpu.gpr[2]==81);
    assert(psx_mod_read_word(CELL_LIST+81*4)==0);
    assert(psx_mod_read_word(CAPTURE_LIST+CAPTURE_CAP*8)==0xABCDEF12);
    uint32_t closest=psx_mod_read_word(CELL_LIST);
    assert(closest>=0x80140000 && closest<0x80140000+129*129*8);
    psx_mod_write_word(0x80021CEC, 0x27BDFF50);
    psx_mod_write_word(0x80022108, 0x290A1000); psx_mod_write_word(0x8002279C, 0x12345678);
    code_writes=0; bypass_subdivision=1; render(&cpu, 0x80021CEC);
    assert(code_writes==0 && psx_mod_read_word(0x80022108)==0x290A1000);
    assert(psx_mod_read_word(0x8002279C)==0x12345678);
    assert(fog_free(&cpu, 0x8007A0C0)); assert(psx_mod_read_word(FOG)==0);
    assert(psx_mod_read_half(0x8011006E)==8192 && psx_mod_read_half(0x8011006C)==4);
    psx_mod_write_word(FOG, 0x80108000); assert(!fog_free(&cpu, 0x8007A0C0));
    puts("MediEvil terrain contracts passed");
    return 0;
}
