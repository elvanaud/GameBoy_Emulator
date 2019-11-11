#include <iostream>
#include "Bus.h"
#include "Z80_Gameboy.h"
#include "PPU_Gameboy.h"

#include <SFML/Graphics.hpp>

int main()
{
    Z80_Gameboy cpu;
    PPU_Gameboy ppu;
    //cpu.disassemble = true;
    Bus gb(cpu,ppu);
    //gb.loadCartridge("ROMS/cpu_instrs/cpu_instrs.gb");
    //gb.loadCartridge("ROMS/cpu_instrs/individual/01-special.gb"); //PASSED
    gb.loadCartridge("ROMS/cpu_instrs/individual/03-op sp,hl.gb");
    //gb.loadCartridge("ROMS/cpu_instrs/individual/04-op r,imm.gb");
    //gb.loadCartridge("ROMS/cpu_instrs/individual/05-op rp.gb"); //PASSED
    //gb.loadCartridge("ROMS/cpu_instrs/individual/06-ld r,r.gb");
    //gb.loadCartridge("ROMS/cpu_instrs/individual/07-jr,jp,call,ret,rst.gb");
    //gb.loadCartridge("ROMS/cpu_instrs/individual/08-misc instrs.gb"); //PASSED
    //gb.loadCartridge("ROMS/cpu_instrs/individual/09-op r,r.gb"); //failed
    //gb.loadCartridge("ROMS/cpu_instrs/individual/10-bit ops.gb");
    //gb.loadCartridge("ROMS/cpu_instrs/individual/11-op a,(hl).gb");

    //gb.loadCartridge("ROMS/warioland.gb");
    //gb.loadCartridge("ROMS/Tetris.GB");
    //gb.loadCartridge("ROMS/Donkey Kong Land (USA, Europe).gb");
    //gb.loadCartridge("ROMS/Super Mario Land (Patch FR).gb");

    gb.disassemble();
    std::cout << std::hex;

    gb.run();
    //std::cout << cpu.assembled;

    return 0;
}
