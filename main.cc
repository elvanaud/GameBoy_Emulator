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

    //gb.loadCartridge("ROMS/Space Invaders (PD).gb");
    //gb.loadCartridge("ROMS/Tetris.GB");
    gb.loadCartridge("ROMS/bpong.gb");
    //gb.loadCartridge("ROMS/Soukoban (J).gb");
    //gb.loadCartridge("ROMS/Mystical Ninja (U) [S][b1].gb");

    //gb.loadCartridge("ROMS/warioland.gb");
    //gb.loadCartridge("ROMS/Donkey Kong Land (USA, Europe).gb");
    //gb.loadCartridge("ROMS/Super Mario Land (Patch FR).gb");


    //gb.disassemble();
    std::cout << std::hex;

    gb.run();
    //std::cout << cpu.assembled;

    return 0;
}
