#include <iostream>
#include "Bus.h"
#include "Z80_Gameboy.h"
#include "PPU_Gameboy.h"
#include "Timer_Gameboy.h"

#include <time.h>

int main(int argc, char **argv)
{
    //srand(time(NULL));
    Z80_Gameboy cpu;
    PPU_Gameboy ppu;
    Timer_Gameboy tim;
    //cpu.disassemble = true;
    Bus gb(cpu,ppu,tim);
    //gb.loadCartridge("ROMS/cpu_instrs/cpu_instrs.gb");
    //gb.loadCartridge("ROMS/cpu_instrs/individual/01-special.gb"); //PASSED
    //gb.loadCartridge("ROMS/cpu_instrs/individual/02-interrupts.gb"); //PASSED
    //gb.loadCartridge("ROMS/cpu_instrs/individual/03-op sp,hl.gb"); //PASSED
    //gb.loadCartridge("ROMS/cpu_instrs/individual/04-op r,imm.gb"); //PASSED
    //gb.loadCartridge("ROMS/cpu_instrs/individual/05-op rp.gb"); //PASSED
    //gb.loadCartridge("ROMS/cpu_instrs/individual/06-ld r,r.gb"); //PASSED
    //gb.loadCartridge("ROMS/cpu_instrs/individual/07-jr,jp,call,ret,rst.gb"); //PASSED
    //gb.loadCartridge("ROMS/cpu_instrs/individual/08-misc instrs.gb"); //PASSED
    //gb.loadCartridge("ROMS/cpu_instrs/individual/09-op r,r.gb"); //PASSED
    //gb.loadCartridge("ROMS/cpu_instrs/individual/10-bit ops.gb"); //PASSED
    //gb.loadCartridge("ROMS/cpu_instrs/individual/11-op a,(hl).gb"); //PASSED
    //gb.loadCartridge("ROMS/instr_timing.gb"); //PASSED
    //gb.loadCartridge("ROMS/01-read_timing.gb");
    //gb.loadCartridge("ROMS/02-write_timing.gb");
    //gb.loadCartridge("ROMS/03-modify_timing.gb");


    //gb.loadCartridge("ROMS/Tetris.GB");
    //gb.loadCartridge("ROMS/DRMARIO.GB");
    //gb.loadCartridge("ROMS/opus5.gb");
    //gb.loadCartridge("ROMS/lyc.gb");
    //gb.loadCartridge("ROMS/bpong.gb");
    //gb.loadCartridge("ROMS/Soukoban (J).gb");


    //All MBC1:
    //gb.loadCartridge("ROMS/warioland.gb");
    gb.loadCartridge("ROMS/LinksAwakening.gb");
    //gb.loadCartridge("ROMS/Donkey Kong Land (USA, Europe).gb");
    //gb.loadCartridge("ROMS/Super Mario Land (Patch FR).gb");
    //gb.loadCartridge("ROMS/Space Invaders (PD).gb");
    //gb.loadCartridge("ROMS/Mystical Ninja (U) [S][b1].gb");
    //gb.loadCartridge("ROMS/Kirby's Dream Land (UE) [b1].gb");
    //gb.loadCartridge("ROMS/PAC-MAN/PAC-MAN.GB");
    //gb.loadCartridge("ROMS/Kirby's Dream Land 2 (U) [S][!].gb");


    //gb.disassemble();
    std::cout << std::hex;

    gb.run();
    //std::cout << cpu.assembled;

    return 0;
}
