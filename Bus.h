#ifndef BUS_H
#define BUS_H

#include <cstdint>
#include <string>
#include <iostream>
#include <sstream>

#include <SFML/Graphics.hpp>

class Z80_Gameboy;
class PPU_Gameboy;
class Timer_Gameboy;

class Bus
{
private:
    Z80_Gameboy& cpu;
    PPU_Gameboy& ppu;
    Timer_Gameboy& tim;

    uint8_t if_reg;
    uint8_t ie;
    uint8_t sb,sc;

    std::string msg = "";
    bool stopMode = false;
    std::string asmArray[0x8000];
    uint8_t ram[0x10000];
    sf::RenderWindow app;
    bool blockMemoryWrite = false;

public:
    Bus(Z80_Gameboy& c,PPU_Gameboy& p,Timer_Gameboy& t);

    void loadCartridge(std::string path);

    void write(uint16_t adr,uint8_t data);
    uint8_t read(uint16_t adr);

    //void triggerInterupt(uint8_t interupt);

    void triggerStopMode(bool stop);

    void run();
    std::string showMemory(uint16_t src,uint16_t dst)
    {
        std::stringstream ss;
        ss << std::showbase << std::hex;
        ss << "Memory Range from "<<src<<" to "<<dst<< "\nAdress: value\n";
        for(uint16_t i = src; i <= dst; i++)
        {
            ss << i << ": " << (int)ram[i]<<"\n";
        }
        return ss.str();
    }

    void disassemble();

    void testProgram()
    {
        uint8_t fibo[21] = {
            0b00'100'001,0x00,0x03,
            0b00'000'110,0x01,
            0b00'001'110,0x01,
            0b00'010'110,0x01,
            0b01'111'000,
            0b10'000'001,
            0b00'100'010,
            0b01'001'000,
            0b01'000'111,
            0b00'010'100,
            0b01'111'010,
            0b11'111'110,10,
            0b11'000'010,0x09,0x01
        };

        uint8_t testDAA[] = {
            0b00'010'001,0x00,0x9a,
            0b11'010'101,
            0b11'110'001,
            0b00'100'111,
            0b00'010'100,
            0b11'000'010,0x03,0x01,
            0b01'111'011,
            0b11'000'110,0x10,
            0b01'011'111,
            0b11'000'010,0x03,0x01
        };

        for(uint16_t i = 0; i < 17; i++)
        {
            ram[i+0x100] = testDAA[i];
        }
    }

    sf::RenderWindow & getWindow()
    {
        return app;
    }
};

#endif // BUS_H
